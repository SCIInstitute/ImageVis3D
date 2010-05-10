/*
   For more information, please see: http://software.sci.utah.edu

   The MIT License

   Copyright (c) 2008 Scientific Computing and Imaging Institute,
   University of Utah.


   Permission is hereby granted, free of charge, to any person obtaining a
   copy of this software and associated documentation files (the "Software"),
   to deal in the Software without restriction, including without limitation
   the rights to use, copy, modify, merge, publish, distribute, sublicense,
   and/or sell copies of the Software, and to permit persons to whom the
   Software is furnished to do so, subject to the following conditions:

   The above copyright notice and this permission notice shall be included
   in all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
   OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
   THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
   FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
   DEALINGS IN THE SOFTWARE.
*/


//!    File   : RenderWindowGL.cpp
//!    Author : Jens Krueger
//!             SCI Institute
//!             University of Utah
//!    Date   : January 2009
//
//!    Copyright (C) 2008 SCI Institute

#include "../Tuvok/StdTuvokDefines.h"
#include <sstream>
#include "GL/glew.h"
#include "RenderWindowGL.h"
#include "ImageVis3D.h"

#include <QtGui/QtGui>
#include <QtGui/QMessageBox>
#include <QtOpenGL/QtOpenGL>
#include "../Tuvok/Controller/Controller.h"
#include "../Tuvok/Renderer/GL/GLFrameCapture.h"
#include "../Tuvok/IO/IOManager.h"

using namespace std;
using namespace tuvok;

std::string RenderWindowGL::ms_glExtString = "";

RenderWindowGL::RenderWindowGL(MasterController& masterController,
                               MasterController::EVolumeRendererType eType,
                               const QString& dataset,
                               unsigned int iCounter,
                               bool bUseOnlyPowerOfTwo,
                               bool bDownSampleTo8Bits,
                               bool bDisableBorder,
                               bool bNoRCClipplanes,
                               QGLWidget* glShareWidget,
                               const QGLFormat& fmt,
                               QWidget* parent,
                               Qt::WindowFlags flags) :
  QGLWidget(fmt, parent, glShareWidget, flags),
  RenderWindow(masterController, eType, dataset, iCounter, parent),
  m_bNoRCClipplanes(bNoRCClipplanes)
{
  if(!SetNewRenderer( bUseOnlyPowerOfTwo, bDownSampleTo8Bits, bDisableBorder))
    return;


  setObjectName("RenderWindowGL");  // this is used by WidgetToRenderWin() to detect the type
  setWindowTitle(m_strID);
  setFocusPolicy(Qt::StrongFocus);
  setMouseTracking(true);

  Initialize(); //finish initializing.
}

bool RenderWindowGL::SetNewRenderer(bool bUseOnlyPowerOfTwo, 
                                    bool bDownSampleTo8Bits,
                                    bool bDisableBorder) {
  m_Renderer = m_MasterController.RequestNewVolumeRenderer(
                  m_eRendererType, bUseOnlyPowerOfTwo, bDownSampleTo8Bits,
                  bDisableBorder, m_bNoRCClipplanes, false
               );
  // so far we are not rendering anything but the volume therefore
  // disable the depth-buffer to offscreen copy operations
  m_Renderer->SetConsiderPreviousDepthbuffer(false);

  if (!m_Renderer->LoadDataset(m_strDataset.toStdString())) {
    m_bRenderSubsysOK = false;
    return false;
  }

  return true;
}

RenderWindowGL::~RenderWindowGL()
{
  // needed for the cleanup call in the parent destructor to work properly
  makeCurrent();

  // ignore mouse/keyboard events while we're killing ourself.
  GetQtWidget()->setEnabled(false);
}

void RenderWindowGL::InitializeRenderer()
{
  static bool bFirstTime = true;
  static bool bRenderSubSysOKFirstTime = true;
  m_bRenderSubsysOK = bRenderSubSysOKFirstTime;

  if (bFirstTime) {
    int err = glewInit();
    if (err != GLEW_OK) {
      T_ERROR("Error initializing GLEW: %s", glewGetErrorString(err));
      m_bRenderSubsysOK = false;
      return;
    } else {
      const GLubyte *vendor=glGetString(GL_VENDOR);
      const GLubyte *renderer=glGetString(GL_RENDERER);
      const GLubyte *version=glGetString(GL_VERSION);
      stringstream s;
      s << vendor << " " << renderer << " with OpenGL version " << version;
      ms_gpuVendorString = s.str();
      MESSAGE("Starting up GL!  Running on a %s", ms_gpuVendorString.c_str());

      bool bOpenGLSO12 = atof((const char*)version) >= 1.2;
      bool bOpenGLSO20 = atof((const char*)version) >= 2.0;
      bool bOpenGLSO   = glewGetExtension("GL_ARB_shader_objects");
      bool bOpenGLSL   = glewGetExtension("GL_ARB_shading_language_100");
      bool bOpenGL3DT  = glewGetExtension("GL_EXT_texture3D");
      bool bOpenGLFBO  = glewGetExtension("GL_EXT_framebuffer_object");

      GLint iMaxVolumeDims;
      if (bOpenGLSO12 || bOpenGL3DT ) {
        glGetIntegerv(GL_MAX_3D_TEXTURE_SIZE_EXT, &iMaxVolumeDims);
        ms_b3DTexInDriver = true;
      } else {
        glGetIntegerv(GL_MAX_TEXTURE_SIZE, &iMaxVolumeDims);
        ms_b3DTexInDriver = false;
      }
      ms_iMaxVolumeDims = iMaxVolumeDims;        

      char *extensions = NULL;
      extensions = (char*)glGetString(GL_EXTENSIONS);
      if (extensions != NULL)  ms_glExtString = extensions;

      if (!bOpenGLSO12 && !bOpenGL3DT) { // according to spec 3D textures
                                         // are part of the OpenGl 1.2 core 
                                         // we may have to change this if we
                                         // realize that too many drivers
                                         // are reporting a GL version greater
                                         // equal to 1.2 but do not support
                                         // 3D textures

        if (m_eRendererType == MasterController::OPENGL_2DSBVR) {
          // hardware does not support 3D textures but the user already
          // selected the 2D stack based volume renderer
          MESSAGE("OpenGL 3D textures not supported (GL_EXT_texture3D). "
                  "This is not an issue as the rendertype is set to "
                  "a 2D stack based renderer.");
        } else {
          // hardware does not support 3D textures but the user 
          // selected a renderer that requires 3D textures
          // so we write out a warning and switch to the 2D texture
          // stack based renderer

          WARNING("OpenGL 3D textures not supported, switching to "
                  "2D texture stack based renderer. To avoid this "
                  "warning and to improve startup times please "
                  "switch to 2D slicing mode in the preferences.");
          bool bUseOnlyPowerOfTwo = m_Renderer->GetUseOnlyPowerOfTwo();
          bool bDownSampleTo8Bits = m_Renderer->GetDownSampleTo8Bits();
          bool bDisableBorder = m_Renderer->GetDisableBorder();
          
          // delete old renderer
          Cleanup();
          // create new renderer that uses only 2D slices
          m_eRendererType = MasterController::OPENGL_2DSBVR;
          SetNewRenderer(bUseOnlyPowerOfTwo,bDownSampleTo8Bits,bDisableBorder);
          Initialize();
        }
      }

      if (bOpenGLFBO && (bOpenGLSO20 || bOpenGLSL)) {

        // A small but still significant subset of cards report that they
        // support 3D textures, as long as they are 0^3 or smaller.  Yeah.
        // All such cards (that we've seen) work fine.  It's a common use
        // case, so we'll skip the warning for now.  -- tjf, Nov 18 2009
        if (ms_iMaxVolumeDims > 0 && ms_iMaxVolumeDims < m_MasterController.IOMan()->GetMaxBrickSize()) {

          std::ostringstream warn;
          warn << "Maximum supported texture size (" << ms_iMaxVolumeDims << ") "
               << "is smaller than the current setting ("
               << m_MasterController.IOMan()->GetMaxBrickSize() << "). "
               << "Adjusting settings!";
          WARNING("%s", warn.str().c_str());

          m_MasterController.IOMan()->SetMaxBrickSize(ms_iMaxVolumeDims);
        } else {
          MESSAGE("Maximum supported texture size: %u "
                  "(required by IO subsystem: %llu)", ms_iMaxVolumeDims,
                  m_MasterController.IOMan()->GetMaxBrickSize());
        }

        m_bRenderSubsysOK = true;
      } else {
        T_ERROR("Insufficient OpenGL support:");

        if (!bOpenGLSO) {
          T_ERROR("OpenGL shader objects not supported "
                  "(GL_ARB_shader_objects)");
        }
        if (!bOpenGLSL) {
          T_ERROR("OpenGL shading language version 1.0 not supported"
                  " (GL_ARB_shading_language_100)");
        }
        if (!bOpenGLFBO) {
          T_ERROR("OpenGL framebuffer objects not supported "
                  "(GL_EXT_framebuffer_object)");
        }
        m_bRenderSubsysOK = false;
      }
    }
  }

  if (m_bRenderSubsysOK) { 
    if (m_Renderer == NULL)
      m_bRenderSubsysOK = false;
    else {
  #ifdef DETECTED_OS_LINUX
      m_Renderer->AddShaderPath("/usr/share/imagevis3d/shaders");
  #endif
      m_bRenderSubsysOK = m_Renderer->Initialize();
    }
  }

  if (!m_bRenderSubsysOK) {
    if (m_Renderer) m_Renderer->Cleanup();
    m_MasterController.ReleaseVolumerenderer(m_Renderer);
    m_Renderer = NULL;
  }

  if (bFirstTime) { 
    bRenderSubSysOKFirstTime = m_bRenderSubsysOK;
    bFirstTime = false;
  }
}

void RenderWindowGL::ForceRepaint() {
  repaint();
}

void RenderWindowGL::SetBlendPrecision(AbstrRenderer::EBlendPrecision eBlendPrecisionMode) {
  makeCurrent();
  RenderWindow::SetBlendPrecision(eBlendPrecisionMode);
}

void RenderWindowGL::ToggleFullscreen() {
    /// \todo find out how to do this in QT, if fixed remember to remove the setVisible(false) in ImageVis3D
}

void RenderWindowGL::PaintOverlays() {
  if (GetViewMode() != VM_SINGLE)
    RenderSeparatingLines();
}


void RenderWindowGL::RenderSeparatingLines() {
  glDisable(GL_BLEND);
  glDisable(GL_DEPTH_TEST);

  glMatrixMode(GL_PROJECTION);
  glPushMatrix();
  glLoadIdentity();
  glOrtho(0, 1, 1, 0, 0, 1); // Note we reverse top and bottom to match the QT canvas
  glMatrixMode(GL_MODELVIEW);
  glPushMatrix();
  glLoadIdentity();

  glLineWidth(regionSplitterWidth);

  if (GetViewMode() == VM_TWOBYTWO) {
    glBegin(GL_LINES);
      glColor4f(1.0f,1.0f,1.0f,1.0f);
      glVertex3f(m_vWinFraction.x,-1,0);
      glVertex3f(m_vWinFraction.x,1,0);
      glVertex3f(-1,m_vWinFraction.y,0);
      glVertex3f(1,m_vWinFraction.y,0);
    glEnd();
  }

  glLineWidth(1);

  glMatrixMode(GL_PROJECTION);
  glPopMatrix();
  glMatrixMode(GL_MODELVIEW);
  glPopMatrix();

}
