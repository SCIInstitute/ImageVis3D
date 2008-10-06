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


//!    File   : RenderWindow.h
//!    Author : Jens Krueger
//!             SCI Institute
//!             University of Utah
//!    Date   : July 2008
//
//!    Copyright (C) 2008 SCI Institute

#pragma once

#ifndef RENDERWINDOW_H
#define RENDERWINDOW_H

#include "Controller/MasterController.h"

#include <QtGui/QListWidget>
#include <QtOpenGL/QGLWidget>


class RenderWindow : public QGLWidget
{
  Q_OBJECT  
  public:
    RenderWindow(MasterController& masterController,
		 QString dataset,
		 unsigned int iCounter,
		 QGLWidget* glWidget,
		 QWidget* parent = 0,
		 Qt::WindowFlags flags = 0);

    virtual ~RenderWindow();

    QString GetDatasetName() {return m_strDataset;}
    QString GetWindowID() {return m_strID;}
    QSize minimumSizeHint() const;
    QSize sizeHint() const;
    AbstrRenderer* GetRenderer() {return m_Renderer;}
    void CheckForRedraw();
    void SetRendermode(ERenderMode eRenderMode) {
      makeCurrent();
      m_Renderer->SetRendermode(eRenderMode); }

    void SetColors(FLOATVECTOR3 vBackColors[2], FLOATVECTOR4 vTextColor) {
      makeCurrent();
      m_Renderer->SetBackgroundColors(vBackColors); 
      m_Renderer->SetTextColor(vTextColor); 
    }

  public slots:
    void ToggleRenderWindowView1x3();
    void ToggleRenderWindowView2x2();
    void ToggleRenderWindowViewSingle();

  signals:
    void RenderWindowViewChanged(int iViewID);
    void WindowActive(RenderWindow* sender);
    void WindowClosing(RenderWindow* sender);

  protected:
    virtual void initializeGL();
    virtual void paintGL();
    virtual void resizeGL(int width, int height);
    virtual void mousePressEvent(QMouseEvent *event);
    virtual void mouseMoveEvent(QMouseEvent *event);
    virtual void closeEvent(QCloseEvent *event);
    virtual void focusInEvent(QFocusEvent * event);
    virtual void Cleanup();

  private:
    GPUSBVR*          m_Renderer;
    MasterController& m_MasterController;
       
    QString m_strDataset;
    QString m_strID;

    FLOATVECTOR2 m_vLastRot;
    QPoint m_vLastPos;
    UINTVECTOR2  m_vWinDim;
};

#endif // RENDERWINDOW_H
