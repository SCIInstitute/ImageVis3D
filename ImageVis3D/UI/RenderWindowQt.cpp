/*
   For more information, please see: http://software.sci.utah.edu

   The MIT License

   Copyright (c) 2008 Scientific Computing and Imaging Institute,
                      University of Utah.
   Copyright (c) 2017 Tom Fogal

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

/** @brief Qt-related calls in RenderWindow.
 *
 * Including both QtGui stuff and GLEW from the same file is taboo, so we
 * separate out this code that needs to interact with the QtGui module. */
#include <QApplication>
#include <QDesktopWidget>
#include <QMdiSubWindow>
#include <QtGui>
#include "ImageVis3D.h"
#include "RenderWindow.h"

void RenderWindow::RotateViewerWithMouse(const INTVECTOR2& viMouseDelta) {
  const int screen = QApplication::desktop()->screenNumber(m_MainWindow);
  const QRect availableRect(QApplication::desktop()->availableGeometry(screen));

  const FLOATVECTOR2 vfMouseDelta(viMouseDelta.x/float(availableRect.width()), 
                                  viMouseDelta.y/float(availableRect.height()));

  RotateViewer(FLOATVECTOR3(-vfMouseDelta.x*450.f, -vfMouseDelta.y*450.f, 0.f));
}

void RenderWindow::LuaResizeWindow(const UINTVECTOR2& newSize) {
  UINTVECTOR2 renderSize = GetRendererSize();
  UINTVECTOR2 windowSize(GetQtWidget()->size().width(), 
                         GetQtWidget()->size().height());

  UINTVECTOR2 winDecoSize = windowSize-renderSize;
  QMdiSubWindow* w = dynamic_cast<QMdiSubWindow*>(GetQtWidget()->parent());
  if(w) {
    w->resize(newSize.x+winDecoSize.x, newSize.y+winDecoSize.y);
  }
}
