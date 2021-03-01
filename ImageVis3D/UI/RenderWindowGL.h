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


//!    File   : RenderWindowGL.h
//!    Author : Jens Krueger
//!             SCI Institute
//!             University of Utah
//!    Date   : January 2009
//
//!    Copyright (C) 2008 SCI Institute

#pragma once

#ifndef RENDERWINDOWGL_H
#define RENDERWINDOWGL_H

#include "../Tuvok/Controller/MasterController.h"


#include "../Tuvok/Basics/ArcBall.h"
#include <string>
#include <StdDefines.h>
#include "RenderWindow.h"

#include <QtOpenGL/QGLWidget>

#include "../Tuvok/LuaScripting/LuaScripting.h"
#include "../Tuvok/LuaScripting/LuaClassRegistration.h"

class MainWindow;

class RenderWindowGL : public QGLWidget, public RenderWindow
{
  Q_OBJECT
  public:
    RenderWindowGL(tuvok::MasterController& masterController,
                 tuvok::MasterController::EVolumeRendererType eType,
                 const QString& dataset,
                 unsigned int iCounter,
                 bool bUseOnlyPowerOfTwo,
                 bool bDownSampleTo8Bits,
                 bool bDisableBorder,
                 QGLWidget* glShareWidget,
                 const QGLFormat& fmt,
                 QWidget* parent,
                 Qt::WindowFlags flags);

    virtual ~RenderWindowGL();
    static const std::string& GetExtString() {return ms_glExtString;}
    virtual void SetBlendPrecision(
        tuvok::AbstrRenderer::EBlendPrecision eBlendPrecisionMode);
    virtual void UpdateWindow() {updateGL();}
    virtual void InitializeContext() { glInit(); }

  protected:
    virtual void ToggleFullscreen();
    virtual void PaintOverlays();
    virtual void RenderSeparatingLines();
    virtual void SwapBuffers();

  private:

    virtual void InitializeRenderer();
    bool SetNewRenderer(bool bUseOnlyPowerOfTwo, 
                        bool bDownSampleTo8Bits,
                        bool bDisableBorder);
    static std::string ms_glExtString;

  // **************** Qt widget connector calls
  protected:
    virtual void ForceRepaint();
    virtual QWidget* GetQtWidget() {return this;}

    virtual void EmitStereoDisabled() {emit StereoDisabled();}
    virtual void EmitRenderWindowViewChanged(int iViewID) {
      emit RenderWindowViewChanged(iViewID);
    }
    virtual void EmitWindowActive() {emit WindowActive(this);}
    virtual void EmitWindowInActive() {emit WindowInActive(this);}
    virtual void EmitWindowClosing() {emit WindowClosing(this);}

  // **************** Qt callbacks
  public:
    QSize minimumSizeHint() const {return QSize(m_vMinSize.x, m_vMinSize.y);}
    QSize sizeHint() const {return QSize(m_vDefaultSize.x, m_vDefaultSize.y);}

  public slots:
    virtual void ToggleRenderWindowView2x2() {
      RenderWindow::ToggleRenderWindowView2x2();
    }
    virtual void ToggleRenderWindowViewSingle() {
      RenderWindow::ToggleRenderWindowViewSingle();
    }
    virtual void SetTimeSlices(unsigned int iActive, unsigned int iInactive) {
      RenderWindow::SetTimeSlices(iActive, iInactive);
    }

  signals:
    void StereoDisabled();
    void RenderWindowViewChanged(int iViewID);
    void WindowActive(RenderWindow* sender);
    void WindowInActive(RenderWindow* sender);
    void WindowClosing(RenderWindow* sender);

  protected:
    virtual void initializeGL() {InitializeRenderer();}
    virtual void paintGL() {PaintRenderer();}
    virtual void resizeGL(int width, int height) {ResizeRenderer(width, height);}

    virtual void mousePressEvent(QMouseEvent *event)    {QGLWidget::mousePressEvent(event);   MousePressEvent(event);}
    virtual void mouseReleaseEvent(QMouseEvent *event)  {QGLWidget::mouseReleaseEvent(event); MouseReleaseEvent(event);}
    virtual void mouseMoveEvent(QMouseEvent *event)     {QGLWidget::mouseMoveEvent(event);    MouseMoveEvent(event);}
    virtual void wheelEvent(QWheelEvent *event)         {QGLWidget::wheelEvent(event);        WheelEvent(event);}
    virtual void closeEvent(QCloseEvent *event)         {QGLWidget::closeEvent(event);        CloseEvent(event);}
    virtual void focusInEvent(QFocusEvent * event)      {QGLWidget::focusInEvent(event);      FocusInEvent(event);}
    virtual void focusOutEvent ( QFocusEvent * event )  {QGLWidget::focusOutEvent(event);     FocusOutEvent(event);}
    virtual void keyPressEvent ( QKeyEvent * event )    {QGLWidget::keyPressEvent(event);     KeyPressEvent(event);}

  // **************** End Qt callbacks
};

#endif // RENDERWINDOWGL_H
