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


//!    File   : MIPRotDialog.cpp
//!    Author : Jens Krueger
//!             SCI Institute
//!             University of Utah
//!    Date   : December 2008
//
//!    Copyright (C) 2008 SCI Institute

#include "MIPRotDialog.h"
#include <QtCore/QFileInfo>

using namespace std;

MIPRotDialog::MIPRotDialog(uint32_t iImages, bool bOrthoView, bool bStereo, bool bUseLOD, uint32_t iEyeDist, QWidget* parent /* = 0 */, Qt::WindowFlags flags /* = 0 */) :
  QDialog(parent, flags)
{

  setupUi(this, iImages, bOrthoView, bStereo, bUseLOD, iEyeDist);
}

MIPRotDialog::~MIPRotDialog(void)
{
}

void MIPRotDialog::setupUi(QDialog *MIPRotDialog, uint32_t iImages, bool bOrthoView, bool bStereo, bool bUseLOD, uint32_t iEyeDist) {
  Ui_MIPRotDialog::setupUi(MIPRotDialog);

  spinBox_Images->setValue(iImages);
  if (bOrthoView)
    radioButton_Ortho->setChecked(true);
  else
    radioButton_Persp->setChecked(true);
  checkBox_Stereo->setChecked(bStereo);
  checkBox_NoLOD->setChecked(!bUseLOD);

  horizontalSlider_EyeDist->setValue(iEyeDist);

  UpdateDegreeLabel();
}


void MIPRotDialog::UpdateDegreeLabel() {
  float fDegreePerImage = 360.0f/spinBox_Images->value();
  QString qstr = tr("(%1° per image)").arg(fDegreePerImage);
  label_Degree->setText(qstr);
  UpdateStereoCheckbox();
}

void MIPRotDialog::UpdateStereoCheckbox() {
  double fDegreePerImage = 360.0/spinBox_Images->value();
  int iEyeDist = horizontalSlider_EyeDist->value();
  int iReuseDist = int(iEyeDist/fDegreePerImage);
  bool bAreImagesReusable = (iReuseDist == iEyeDist/fDegreePerImage);

  if (checkBox_Stereo->isChecked() && !bAreImagesReusable) {
    checkBox_Stereo->setText("Stereo (performance warning)");
  } else {
    checkBox_Stereo->setText("Stereo");
  }
}

void MIPRotDialog::UpdateEyeDistLabel() {
  QString qstr = tr("(%1°)").arg(horizontalSlider_EyeDist->value());
  label_EyeDist->setText(qstr);
  UpdateStereoCheckbox();
}



uint32_t MIPRotDialog::GetNumImages() const {
  return uint32_t(spinBox_Images->value());
}

bool MIPRotDialog::GetUseOrtho() const {
  return radioButton_Ortho->isChecked();
}

bool MIPRotDialog::GetUseStereo() const {
  return checkBox_Stereo->isChecked();
}

bool MIPRotDialog::GetUseLOD() const {
  return !checkBox_NoLOD->isChecked();
}

uint32_t MIPRotDialog::GetEyeDist() const {
  return uint32_t(horizontalSlider_EyeDist->value());
}
