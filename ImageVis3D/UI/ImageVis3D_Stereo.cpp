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


//!    File   : ImageVis3D_Progress.cpp
//!    Author : Jens Krueger
//!             SCI Institute
//!             University of Utah
//!    Date   : November 2008
//
//!    Copyright (C) 2008 SCI Institute

#include "ImageVis3D.h"

using namespace std;

void MainWindow::ToggleStereoRendering() {
  if (m_ActiveRenderWin == NULL) return;  
  m_ActiveRenderWin->GetRenderer()->SetStereo(checkBox_Stereo->isChecked());
}

void MainWindow::SetStereoEyeDistance() {
  if (m_ActiveRenderWin == NULL) return;
  m_ActiveRenderWin->GetRenderer()->SetStereoEyeDist(float(horizontalSlider_EyeDistance->value())/100.0);
}

void MainWindow::SetStereoFocalLength() {
  if (m_ActiveRenderWin == NULL) return;  
  m_ActiveRenderWin->GetRenderer()->SetStereoFocalLength(float(horizontalSlider_FocalLength->value())/10.0);
}
