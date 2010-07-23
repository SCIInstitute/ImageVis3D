/*
   For more information, please see: http://software.sci.utah.edu

   The MIT License

   Copyright (c) 2010 Interactive Visualization and Data Analysis Group.


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

//!    File   : ScaleAndBiasDlg.cpp
//!    Author : Jens Krueger
//!             IVCI & DFKI & MMCI, Saarbruecken
//!             SCI Institute, University of Utah
//!    Date   : July 2010
//
//!    Copyright (C) 2010 DFKI, MMCI, SCI Institute

#include "ScaleAndBiasDlg.h"

using namespace std;

ScaleAndBiasDlg::ScaleAndBiasDlg(const std::string& strDesc, 
                                 const FLOATVECTOR3& min, 
                                 const FLOATVECTOR3& max, 
                                 QWidget* parent /* = 0 */, 
                                 Qt::WindowFlags flags /* = 0 */) :
  QDialog(parent, flags),
  m_min(min),
  m_max(max)
{
  setupUi(this, strDesc);
}

ScaleAndBiasDlg::~ScaleAndBiasDlg(void)
{
}

void ScaleAndBiasDlg::ScaleIsotropic() {
  FLOATVECTOR3 s = (m_max-m_min);
  FLOATVECTOR3 scaleVec = 1.0f/s;

  float scale = scaleVec.minVal();
  FLOATVECTOR3 c = ((m_max+m_min)/2.0f)*scale;
  FLOATVECTOR3 biasVec = -c;

  doubleSpinBox_Sx->setValue(scale);
  doubleSpinBox_Sy->setValue(scale);
  doubleSpinBox_Sz->setValue(scale);

  doubleSpinBox_Bx->setValue(biasVec.x);
  doubleSpinBox_By->setValue(biasVec.y);
  doubleSpinBox_Bz->setValue(biasVec.z);

  ValuesChanged();
}

void ScaleAndBiasDlg::ScaleUnisotropic() {
  FLOATVECTOR3 s = (m_max-m_min);
  FLOATVECTOR3 scaleVec = 1.0f/s;
  FLOATVECTOR3 c = ((m_max+m_min)/2.0f)*scaleVec;
  FLOATVECTOR3 biasVec = -c;

  doubleSpinBox_Sx->setValue(scaleVec.x);
  doubleSpinBox_Sy->setValue(scaleVec.y);
  doubleSpinBox_Sz->setValue(scaleVec.z);

  doubleSpinBox_Bx->setValue(biasVec.x);
  doubleSpinBox_By->setValue(biasVec.y);
  doubleSpinBox_Bz->setValue(biasVec.z);

  ValuesChanged();
}

void ScaleAndBiasDlg::ValuesChanged() {
  scaleVec = FLOATVECTOR3(doubleSpinBox_Sx->value(),
                          doubleSpinBox_Sy->value(),
                          doubleSpinBox_Sz->value());
  biasVec = FLOATVECTOR3(doubleSpinBox_Bx->value(),
                         doubleSpinBox_By->value(),
                         doubleSpinBox_Bz->value());

  UpdatePostSize();
}


void ScaleAndBiasDlg::UpdatePostSize() {
  FLOATVECTOR3 c = (m_max+m_min)/2.0f;
  FLOATVECTOR3 s = (m_max-m_min);

  c = c*scaleVec + biasVec;
  s = s*scaleVec;
  
  QString text = tr("After scale and bias: "
                    "Size: [%1, %2, %3] "
                    "Center: [%4, %5, %6]").arg(s.x,0,'f',3).arg(s.y,0,'f',3).arg(s.z,0,'f',3)
                                           .arg(c.x,0,'f',3).arg(c.y,0,'f',3).arg(c.z,0,'f',3);
  label_targetDim->setText(text);

}

void ScaleAndBiasDlg::setupUi(QDialog *ScaleAndBiasDlg, const std::string& strDesc) {
  Ui_ScaleAndBiasDlg::setupUi(ScaleAndBiasDlg);

  FLOATVECTOR3 c = (m_max+m_min)/2.0f;
  FLOATVECTOR3 s = m_max-m_min;

  // pre scale size
  QString text = tr("Before scale and bias: "
                    "Size: [%1, %2, %3] "
                    "Center: [%4, %5, %6]").arg(s.x,0,'f',3).arg(s.y,0,'f',3).arg(s.z,0,'f',3)
                                           .arg(c.x,0,'f',3).arg(c.y,0,'f',3).arg(c.z,0,'f',3);
  label_currentDim->setText(text);

  label_meshDesc->setText(strDesc.c_str());
  ValuesChanged();
}
