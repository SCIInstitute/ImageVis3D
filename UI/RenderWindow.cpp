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


//!    File   : RenderWindow.cpp
//!    Author : Jens Krueger
//!             SCI Institute
//!             University of Utah
//!    Date   : July 2008
//
//!    Copyright (C) 2008 SCI Institute

#include "RenderWindow.h"

#include <QtGui/QtGui>
#include <QtOpenGL/QtOpenGL>
#include <assert.h>

RenderWindow::RenderWindow(MasterController& masterController, QString dataset, unsigned int iCounter, QGLWidget* glShareWidget, QWidget* parent, Qt::WindowFlags flags) :
  QGLWidget(parent, glShareWidget, flags),
  m_Renderer((GPUSBVR*)masterController.RequestNewVolumerenderer(OPENGL_SBVR)),
  m_MasterController(masterController),
  m_strDataset(dataset),
  m_vLastRot(0,0),
  m_vLastPos(0,0),
  m_vWinDim(0,0)
{  
  m_strID = tr("[%1] %2").arg(iCounter).arg(m_strDataset);
  setWindowTitle(m_strID);

  m_Renderer->LoadDataset(m_strDataset.toStdString());
  m_Renderer->SetCurrentView(0);

  this->setFocusPolicy(Qt::StrongFocus);
}

RenderWindow::~RenderWindow()
{
  Cleanup();
}

QSize RenderWindow::minimumSizeHint() const
{
  return QSize(50, 50);
}

QSize RenderWindow::sizeHint() const
{
  return QSize(400, 400);
}

void RenderWindow::initializeGL()
{
  static bool bFirstTime = true;
  if (bFirstTime) {
    int err = glewInit();
    if (err != GLEW_OK) {
      m_MasterController.DebugOut()->Error("RenderWindow::initializeGL", "Error initializing GLEW: %s",glewGetErrorString(err));
    } else {
      const GLubyte *vendor=glGetString(GL_VENDOR);
      const GLubyte *renderer=glGetString(GL_RENDERER);
      const GLubyte *version=glGetString(GL_VERSION);
      m_MasterController.DebugOut()->Message("RenderWindow::initializeGL", "Starting up GL! Running on a %s %s with OpenGL version %s",vendor, renderer, version);
    }

    bFirstTime = false;
  }

  if (m_Renderer != NULL) m_Renderer->Initialize();
}

void RenderWindow::paintGL()
{
  if (m_Renderer != NULL) m_Renderer->Paint();
}

void RenderWindow::resizeGL(int width, int height)
{
  m_vWinDim = UINTVECTOR2((unsigned int)width, (unsigned int)height);
  if (m_Renderer != NULL) m_Renderer->Resize(width, height);
}

void RenderWindow::mousePressEvent(QMouseEvent *event)
{
  m_vLastPos = event->pos();

  if (event->buttons() & Qt::RightButton) {
    if (m_Renderer != NULL) m_Renderer->SetCurrentView((m_Renderer->GetCurrentView()+1) %3);
    emit RenderWindowViewChanged(m_Renderer->GetCurrentView());
    updateGL();
  }
}


float scale(int max, float w) {
	if (w < 0) {
		return w-(((int)w/max)-1)*max;
	} else {
		return w-((int)w/max)*max;
	}
}

void RenderWindow::mouseMoveEvent(QMouseEvent *event)
{
  INTVECTOR2 vDelta (event->x() - m_vLastPos.x(), event->y() - m_vLastPos.y());

  if (event->buttons() & Qt::LeftButton) {

    FLOATVECTOR2 currentRot(scale(2*3.1415f,m_vLastRot.x+2*3.1415f*(vDelta.x)/m_vWinDim.x), scale(2*3.1415f,m_vLastRot.y+2*3.1415f*(vDelta.y)/m_vWinDim.y));

    if (m_vLastRot != currentRot) {
      m_vLastRot = currentRot;
      if (m_Renderer != NULL) m_Renderer->SetRotation(currentRot);
      updateGL();
    }
  }
  m_vLastPos = event->pos();
}

void RenderWindow::ToggleRenderWindowView1x3() {
  if (m_Renderer != NULL) m_Renderer->SetCurrentView(0);
  emit RenderWindowViewChanged(0);
  updateGL();
}

void RenderWindow::ToggleRenderWindowView2x2() {
  if (m_Renderer != NULL) m_Renderer->SetCurrentView(1);
  emit RenderWindowViewChanged(1);
  updateGL();
}

void RenderWindow::ToggleRenderWindowViewSingle() {
  if (m_Renderer != NULL) m_Renderer->SetCurrentView(2);
  emit RenderWindowViewChanged(2);
  updateGL();
}

void RenderWindow::Cleanup() {
  if (m_Renderer == NULL) return;
  
  makeCurrent();
  m_Renderer->Cleanup();
  m_MasterController.ReleaseVolumerenderer(m_Renderer);
  m_Renderer = NULL;
}

void RenderWindow::closeEvent(QCloseEvent *event) {
  QGLWidget::closeEvent(event);

  emit WindowClosing(this);
}

void RenderWindow::focusInEvent ( QFocusEvent * event ) {
  // call superclass method
  QGLWidget::focusInEvent(event);

  if (event->gotFocus()) {
    emit WindowActive(this);
  }
}

void RenderWindow::CheckForRedraw() {
  makeCurrent();
  if (m_Renderer->CheckForRedraw()) {
    update();
  }
}
