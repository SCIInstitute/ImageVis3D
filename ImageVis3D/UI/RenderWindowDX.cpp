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


//!    File   : RenderWindowDX.cpp
//!    Author : Jens Krueger
//!             SCI Institute
//!             University of Utah
//!    Date   : January 2009
//
//!    Copyright (C) 2008 SCI Institute

#include "../Tuvok/StdTuvokDefines.h"
#if defined(_WIN32) && defined(USE_DIRECTX)
#include <cassert>
#include <sstream>
/// @todo FIXME -- remove (obviously)!
/// We need this first because GPUMemMan depends on OpenGL
/// but shouldn't, and GLEW will blow up if we don't include
/// GLEW before GL includes.
#include "3rdParty/GLEW/GL/glew.h"

#include "RenderWindowDX.h"
#include "ImageVis3D.h"
#include "../Tuvok/Renderer/DX/DXRenderer.h"

#include <QtGui/QtGui>

using namespace std;

RenderWindowDX::RenderWindowDX(MasterController& masterController,
                               MasterController::EVolumeRendererType eType,
                               const QString& dataset,
                               unsigned int iCounter,
                               bool bUseOnlyPowerOfTwo,
                               bool bDownSampleTo8Bits,
                               bool bDisableBorder,
                               QWidget* parent,
                               Qt::WindowFlags flags) :
  QWidget(parent, flags),
  RenderWindow(masterController, eType, dataset, iCounter, parent)
{
  setBaseSize( sizeHint() );

  shared_ptr<LuaScripting> ss = m_MasterController.LuaScript();
  m_LuaAbstrRenderer = ss->cexecRet<LuaClassInstance>(
      "tuvok.renderer.new",
      eType, bUseOnlyPowerOfTwo, bDownSampleTo8Bits,
      bDisableBorder, false, false);
  m_Renderer = m_LuaAbstrRenderer.getRawPointer<AbstrRenderer>(ss);

  string rn = m_LuaAbstrRenderer.fqName();

  // so far we are not rendering anything previous to this renderer 
  // so we can disable the depth-buffer to offscreen copy operations
  ss->cexec(rn + ".setConsiderPrevDepthBuffer", false);

  if (m_LuaAbstrRenderer.isValid(ss)) {
    // hand over the handle of the window we are sitting in not the widget 
    // inside the window
    // setWinID is a function that has been specifically registered by
    // DXRenderer. HWND type is handled as a special case (see DXRenderer.h).
    ss->cexec(rn + ".setWinID", parent->winId());
    if (!ss->cexecRet<bool>(rn + ".loadDataset", m_strDataset.toStdWString())) {
      InitializeRenderer();
      Initialize();

      setObjectName("RenderWindowDX");  // this is used by WidgetToRenderWin() to detect the type
      setWindowTitle(m_strID);
      setFocusPolicy(Qt::StrongFocus);
      setMouseTracking(true);

      m_bRenderSubsysOK = true;
      return;
    }
  }
  m_bRenderSubsysOK = false;
}

RenderWindowDX::~RenderWindowDX()
{
  // ignore mouse/keyboard events while we're killing ourself.
  GetQtWidget()->setEnabled(false);
}

void RenderWindowDX::PaintOverlays() {

}

void RenderWindowDX::RenderSeparatingLines() {

}

void RenderWindowDX::InitializeRenderer()
{
  shared_ptr<LuaScripting> ss = m_MasterController.LuaScript();
  string rn = m_LuaAbstrRenderer.fqName();

  if (!m_LuaAbstrRenderer.isValid(ss))
  {
    m_bRenderSubsysOK = false;
  }
  else
  {
    m_bRenderSubsysOK = ss->cexecRet<bool>(rn + ".initializeDirectX");
  }

  if (!m_bRenderSubsysOK) {
    if (m_LuaAbstrRenderer.isValid(ss)) {
      ss->cexec(rn + ".cleanup");
    }
    m_MasterController.ReleaseVolumeRenderer(m_LuaAbstrRenderer);
    m_LuaAbstrRenderer.invalidate();
  }
}

void RenderWindowDX::ForceRepaint() {
  repaint();
}

void RenderWindowDX::ToggleFullscreen() {
  // TODO
}

void RenderWindowDX::resizeEvent ( QResizeEvent * event ) {
  ResizeRenderer(event->size().width(), event->size().height());
}

#endif // _WIN32 && USE_DIRECTX
