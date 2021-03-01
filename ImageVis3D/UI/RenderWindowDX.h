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


//!    File   : RenderWindowDX.h
//!    Author : Jens Krueger
//!             SCI Institute
//!             University of Utah
//!    Date   : January 2009
//
//!    Copyright (C) 2008 SCI Institute

#if defined(_WIN32) && defined(USE_DIRECTX)

#pragma once

#ifndef RENDERWINDOWDX_H
#define RENDERWINDOWDX_H

#include "../Tuvok/Controller/MasterController.h"
#include "../Tuvok/Basics/DynamicDX.h"
#include "../Tuvok/Basics/ArcBall.h"

#include <string>
#include <StdDefines.h>
#include "RenderWindow.h"

using namespace tuvok;

class MainWindow;

class RenderWindowDX : public QWidget, public RenderWindow
{
  Q_OBJECT
  public:
    RenderWindowDX(MasterController& masterController,
                 MasterController::EVolumeRendererType eType,
                 const QString& dataset,
                 unsigned int iCounter,
                 bool bUseOnlyPowerOfTwo,
                 bool bDownSampleTo8Bits,
                 bool bDisableBorder,
                 QWidget* parent,
                 Qt::WindowFlags flags);

    virtual ~RenderWindowDX();

    virtual void UpdateWindow() {update();}
    /// Should initialize w/o render; hard to implement here?
    virtual void InitializeContext() { update(); }

 protected:
    virtual void ToggleFullscreen();
    virtual void PaintOverlays();
    virtual void RenderSeparatingLines();

  private:
    virtual void InitializeRenderer();

  protected slots:
    virtual void resizeEvent ( QResizeEvent * event );


  // **************** Qt widget connector calls
  protected:
    virtual void ForceRepaint();
    virtual QWidget* GetQtWidget() {return this;}

    virtual void EmitStereoDisabled() {emit StereoDisabled();}
    virtual void EmitRenderWindowViewChanged(int iViewID) {emit RenderWindowViewChanged(iViewID);}
    virtual void EmitWindowActive() {emit WindowActive(this);}
    virtual void EmitWindowInActive() {emit WindowInActive(this);}
    virtual void EmitWindowClosing() {emit WindowClosing(this);}

  // **************** Qt callbacks
  public:
    QSize minimumSizeHint() const {return QSize(m_vMinSize.x, m_vMinSize.y);}
    QSize sizeHint() const {return QSize(m_vDefaultSize.x, m_vDefaultSize.y);}

  public slots:
    virtual void ToggleRenderWindowView2x2() {RenderWindow::ToggleRenderWindowView2x2();}
    virtual void ToggleRenderWindowViewSingle() {RenderWindow::ToggleRenderWindowViewSingle();}
    virtual void SetTimeSlices(unsigned int iActive, unsigned int iInactive) {RenderWindow::SetTimeSlices(iActive, iInactive);}

  signals:
    void StereoDisabled();
    void RenderWindowViewChanged(int iViewID);
    void WindowActive(RenderWindow* sender);
    void WindowInActive(RenderWindow* sender);
    void WindowClosing(RenderWindow* sender);

  protected:
    virtual void mousePressEvent(QMouseEvent *event)    {QWidget::mousePressEvent(event);   MousePressEvent(event);}
    virtual void mouseReleaseEvent(QMouseEvent *event)  {QWidget::mouseReleaseEvent(event); MouseReleaseEvent(event);}
    virtual void mouseMoveEvent(QMouseEvent *event)     {QWidget::mouseMoveEvent(event);    MouseMoveEvent(event);}
    virtual void wheelEvent(QWheelEvent *event)         {QWidget::wheelEvent(event);        WheelEvent(event);}
    virtual void closeEvent(QCloseEvent *event)         {QWidget::closeEvent(event);        CloseEvent(event);}
    virtual void focusInEvent(QFocusEvent * event)      {QWidget::focusInEvent(event);      FocusInEvent(event);}
    virtual void focusOutEvent ( QFocusEvent * event )  {QWidget::focusOutEvent(event);     FocusOutEvent(event);}
    virtual void keyPressEvent ( QKeyEvent * event )    {QWidget::keyPressEvent(event);     KeyPressEvent(event);}

  // **************** End Qt callbacks
};

#endif // RENDERWINDOWDX_H
#endif // _WIN32 && USE_DIRECTX
