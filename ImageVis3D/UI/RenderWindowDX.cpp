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

#if defined(_WIN32) && defined(USE_DIRECTX)


#include "RenderWindowDX.h"
#include "ImageVis3D.h"
#include "../Tuvok/Renderer/DX/DXRenderer.h"

#include <QtGui/QtGui>
#include <assert.h>
#include <sstream>

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
  m_Renderer = masterController.RequestNewVolumerenderer(eType, bUseOnlyPowerOfTwo, bDownSampleTo8Bits, bDisableBorder);

  if (m_Renderer) {
    ((DXRenderer*)m_Renderer)->SetWinID(parent->winId());  // hand over the handle of the window we are sitting in not the widget inside the window
    m_Renderer->LoadDataset(m_strDataset.toStdString());
    InitializeRenderer();
    SetupArcBall();
  } else m_bRenderSubsysOK = false;

  setObjectName("RenderWindowDX");  // this is used by WidgetToRenderWin() to detect the type
  setWindowTitle(m_strID);
  setFocusPolicy(Qt::StrongFocus);
  setMouseTracking(true);
}

RenderWindowDX::~RenderWindowDX()
{
  // ignore mouse/keyboard events while we're killing ourself.
  GetQtWidget()->setEnabled(false);
}

void RenderWindowDX::InitializeRenderer()
{
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

void RenderWindowDX::ForceRepaint() {
  repaint();
#ifdef TUVOK_OS_APPLE
  QCoreApplication::processEvents();
#endif
}

void RenderWindowDX::ToggleFullscreen() {
  // TODO
}

void RenderWindowDX::resizeEvent ( QResizeEvent * event ) {
  ResizeRenderer(event->size().width(), event->size().height());
}

#endif // _WIN32 && USE_DIRECTX
