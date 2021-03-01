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
#include <QtWidgets/QMessageBox>

using namespace std;
using namespace tuvok;

void MainWindow::ToggleStereoRendering() {
  if (m_pActiveRenderWin) {
    if (m_pActiveRenderWin->GetViewMode() != RenderWindow::VM_SINGLE ||
        !m_pActiveRenderWin->IsRegion3D(m_pActiveRenderWin->GetActiveRenderRegions()[0])) {
      QString strText = "Stereo rendering is only available in single view 3D mode. Do you want to change to that view now?";
      if (QMessageBox::Yes == QMessageBox::question(this, "3D Stereo", strText, QMessageBox::Yes, QMessageBox::No)) {
        if (!m_pActiveRenderWin->SetRenderWindowView3D()) {
          QMessageBox::information(this, "3D Stereo", "Unable to switch to 3D view, aborting stereo.");
          checkBox_Stereo->setChecked(false);
          return;
        }
      } else {
        checkBox_Stereo->setChecked(false);
        return;
      }
    }
    m_pActiveRenderWin->SetRendererStereoEnabled(checkBox_Stereo->isChecked());
  } else {
    checkBox_Stereo->setChecked(false);
  }
}

void MainWindow::SetStereoEyeDistance() {
  if (m_pActiveRenderWin == NULL) return;
  m_pActiveRenderWin->SetRendererStereoEyeDist(float(horizontalSlider_EyeDistance->value())/100.0);
}

void MainWindow::SetStereoFocalLength() {
  if (m_pActiveRenderWin == NULL) return;
  m_pActiveRenderWin->SetRendererStereoFocalLength(float(horizontalSlider_FocalLength->value())/10.0);
}

void MainWindow::ToggleStereoMode() {
  if (m_pActiveRenderWin == NULL) return;
  if (radioButton_RBStereo->isChecked()) 
    m_pActiveRenderWin->SetRendererStereoMode(AbstrRenderer::SM_RB);
  else if (radioButton_ScanlineStereo->isChecked()) 
    m_pActiveRenderWin->SetRendererStereoMode(AbstrRenderer::SM_SCANLINE);
  else if (radioButton_SideBySide->isChecked()) 
    m_pActiveRenderWin->SetRendererStereoMode(AbstrRenderer::SM_SBS);
  else 
    m_pActiveRenderWin->SetRendererStereoMode(AbstrRenderer::SM_AF);
}

void MainWindow::ToggleStereoEyeSwap() {
  if (m_pActiveRenderWin == NULL) return;
  m_pActiveRenderWin->SetRendererStereoEyeSwap(checkBox_EyeSwap->isChecked());
}
