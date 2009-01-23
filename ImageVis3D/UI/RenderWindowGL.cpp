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

#include "RenderWindowGL.h"
#include "ImageVis3D.h"

#include <QtGui/QtGui>
#include <QtOpenGL/QtOpenGL>
#include <assert.h>
#include <sstream>
#include "../Tuvok/Renderer/GL/GLFrameCapture.h"

using namespace std;

std::string RenderWindowGL::ms_glExtString = "";

RenderWindowGL::RenderWindowGL(MasterController& masterController,
                               MasterController::EVolumeRendererType eType,
                               const QString& dataset,
                               unsigned int iCounter,
                               bool bUseOnlyPowerOfTwo,
                               bool bDownSampleTo8Bits,
                               bool bDisableBorder,
                               QGLWidget* glShareWidget,
                               const QGLFormat& fmt,
                               QWidget* parent,
                               Qt::WindowFlags flags) :
  QGLWidget(fmt, parent, glShareWidget, flags),
  RenderWindow(masterController, eType, dataset, iCounter, parent)
{
  m_Renderer = masterController.RequestNewVolumerenderer(eType, bUseOnlyPowerOfTwo, bDownSampleTo8Bits, bDisableBorder);
  m_Renderer->LoadDataset(m_strDataset.toStdString());  
  SetupArcBall();

  setObjectName("RenderWindowGL");  // this is used by WidgetToRenderWin() to detect the type
  setWindowTitle(m_strID);
  setFocusPolicy(Qt::StrongFocus);
  setMouseTracking(true);
}

RenderWindowGL::~RenderWindowGL() 
{
  makeCurrent();
}

void RenderWindowGL::InitializeRenderer()
{
  static bool bFirstTime = true;
  if (bFirstTime) {
    int err = glewInit();
    if (err != GLEW_OK) {
      m_MasterController.DebugOut()->Error("RenderWindowGL::InitializeRenderer", "Error initializing GLEW: %s",glewGetErrorString(err));
      m_bRenderSubsysOK = false;
      return;
    } else {
      const GLubyte *vendor=glGetString(GL_VENDOR);
      const GLubyte *renderer=glGetString(GL_RENDERER);
      const GLubyte *version=glGetString(GL_VERSION);
      stringstream s;
      s << vendor << " " << renderer << " with OpenGL version " << version;
      ms_gpuVendorString = s.str();
      m_MasterController.DebugOut()->Message("RenderWindowGL::InitializeRenderer", "Starting up GL! Running on a %s", ms_gpuVendorString.c_str());

      bool bOpenGLSO20 = atof((const char*)version) >= 2.0;
      bool bOpenGLSO   = glewGetExtension("GL_ARB_shader_objects");
      bool bOpenGLSL   = glewGetExtension("GL_ARB_shading_language_100");
      bool bOpenGL3DT  = glewGetExtension("GL_EXT_texture3D");
      bool bOpenGLFBO  = glewGetExtension("GL_EXT_framebuffer_object");

      if (bOpenGL3DT) { 
        GLint iMax3DTexDims;
        glGetIntegerv(GL_MAX_3D_TEXTURE_SIZE_EXT, &iMax3DTexDims);        
        ms_iMax3DTexDims = iMax3DTexDims;
      }

      char *extensions = NULL;
      if (glGetString != NULL) extensions = (char*)glGetString(GL_EXTENSIONS);
      if (extensions != NULL)  ms_glExtString = extensions;

      if (bOpenGLFBO && (bOpenGLSO20 || (bOpenGLSL && bOpenGL3DT))) {

        if (ms_iMax3DTexDims < BRICKSIZE) {
          m_MasterController.DebugOut()->Warning("RenderWindowGL::InitializeRenderer", "Maximum supported texture size (%i) is smaller than required by the IO subsystem (%i).", ms_iMax3DTexDims, int(BRICKSIZE));
        } else {
          m_MasterController.DebugOut()->Message("RenderWindowGL::InitializeRenderer", "Maximum supported texture size %i (required by the IO subsystem %i).", ms_iMax3DTexDims, int(BRICKSIZE));
        } 

        m_bRenderSubsysOK = true;
      } else {      
        m_MasterController.DebugOut()->Error("RenderWindowGL::InitializeRenderer", "Insufficient OpenGL support:");
     
        if (!bOpenGLSO) m_MasterController.DebugOut()->Error("RenderWindowGL::InitializeRenderer", "OpenGL shader objects not suported (GL_ARB_shader_objects)");
        if (!bOpenGLSL) m_MasterController.DebugOut()->Error("RenderWindowGL::InitializeRenderer", "OpenGL shading language version 1.0 not suported (GL_ARB_shading_language_100)");
        if (!bOpenGL3DT) m_MasterController.DebugOut()->Error("RenderWindowGL::InitializeRenderer", "OpenGL 3D textures not suported (GL_EXT_texture3D)");
        if (!bOpenGLFBO) m_MasterController.DebugOut()->Error("RenderWindowGL::InitializeRenderer", "OpenGL framebuffer objects not suported (GL_EXT_framebuffer_object)");
        
        m_bRenderSubsysOK = false;
      }
    }

    bFirstTime = false;
  }

  if (m_Renderer == NULL) 
    m_bRenderSubsysOK = false;
  else
    m_bRenderSubsysOK = m_Renderer->Initialize();


  if (!m_bRenderSubsysOK) {
    m_Renderer->Cleanup();
    m_MasterController.ReleaseVolumerenderer(m_Renderer);
    m_Renderer = NULL;
  } 
}

void RenderWindowGL::ForceRepaint() {
  repaint();
#ifdef TUVOK_OS_APPLE
  QCoreApplication::processEvents();
#endif
}

void RenderWindowGL::SetBlendPrecision(AbstrRenderer::EBlendPrecision eBlendPrecisionMode) {
  makeCurrent();
  RenderWindow::SetBlendPrecision(eBlendPrecisionMode);
}
