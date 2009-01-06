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

MIPRotDialog::MIPRotDialog(UINT32 iImages, bool bOrthoView, bool bStereo, QWidget* parent /* = 0 */, Qt::WindowFlags flags /* = 0 */) : 
  QDialog(parent, flags)
{

  setupUi(this, iImages, bOrthoView, bStereo);
}

MIPRotDialog::~MIPRotDialog(void)
{
}

void MIPRotDialog::setupUi(QDialog *MIPRotDialog, UINT32 iImages, bool bOrthoView, bool bStereo) {
  Ui_MIPRotDialog::setupUi(MIPRotDialog);

  spinBox_Images->setValue(iImages);
  if (bOrthoView) 
    radioButton_Ortho->setChecked(true);
  else
    radioButton_Persp->setChecked(true);
  checkBox_Stereo->setChecked(bStereo);

  UpdateDegreeLabel();
}


void MIPRotDialog::UpdateDegreeLabel() {
  float fDegreePerImage = 360.0f/spinBox_Images->value();
  QString qstr = tr("(%1° per image)").arg(fDegreePerImage);
  label_Degree->setText(qstr);
  UpdateStereoCheckbox();
}

void MIPRotDialog::UpdateStereoCheckbox() {
  if (checkBox_Stereo->isChecked() && (spinBox_Images->value() % 120 != 0)) {
    checkBox_Stereo->setText("Stereo (performance warning: image count is not a multiple of 120)");
  } else {
    checkBox_Stereo->setText("Stereo");
  }
}


UINT32 MIPRotDialog::GetNumImages() {
  return UINT32(spinBox_Images->value());
}

bool MIPRotDialog::GetUseOrtho() {
  return radioButton_Ortho->isChecked();
}

bool MIPRotDialog::GetUseStereo() {
  return checkBox_Stereo->isChecked();
}

