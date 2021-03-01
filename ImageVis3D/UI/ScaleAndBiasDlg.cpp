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
#include "../Tuvok/Renderer/RenderMesh.h"
#include <QtCore/QSettings>

using namespace std;

ScaleAndBiasDlg::ScaleAndBiasDlg(shared_ptr<tuvok::RenderMesh> mesh,
                                 size_t index,
                                 const FLOATVECTOR3& vmin, 
                                 const FLOATVECTOR3& vmax,
                                 QWidget* parent /* = 0 */, 
                                 Qt::WindowFlags flags /* = 0 */) :
  QDialog(parent, flags),
  m_index(index),
  m_pMesh(mesh),
  m_min(mesh->GetMin()),
  m_max(mesh->GetMax()),
  m_minVolume(vmin),
  m_maxVolume(vmax)
{
  setupUi(this, QString::fromStdWString(mesh->Name()));
}

ScaleAndBiasDlg::~ScaleAndBiasDlg(void)
{
}

void ScaleAndBiasDlg::ScaleIsotropic() {
  FLOATVECTOR3 s = (m_max-m_min);
  FLOATVECTOR3 _scaleVec = 1.0f/s;

  float scale = _scaleVec.minVal();
  FLOATVECTOR3 c = ((m_max+m_min)/2.0f)*scale;
  FLOATVECTOR3 _biasVec = -c;

  doubleSpinBox_Sx->setValue(scale);
  doubleSpinBox_Sy->setValue(scale);
  doubleSpinBox_Sz->setValue(scale);

  doubleSpinBox_Bx->setValue(_biasVec.x);
  doubleSpinBox_By->setValue(_biasVec.y);
  doubleSpinBox_Bz->setValue(_biasVec.z);

  ValuesChanged();
}

void ScaleAndBiasDlg::ScaleUnisotropic() {
  FLOATVECTOR3 s = (m_max-m_min);
  FLOATVECTOR3 _scaleVec = 1.0f/s;
  FLOATVECTOR3 c = ((m_max+m_min)/2.0f)*_scaleVec;
  FLOATVECTOR3 _biasVec = -c;

  doubleSpinBox_Sx->setValue(_scaleVec.x);
  doubleSpinBox_Sy->setValue(_scaleVec.y);
  doubleSpinBox_Sz->setValue(_scaleVec.z);

  doubleSpinBox_Bx->setValue(_biasVec.x);
  doubleSpinBox_By->setValue(_biasVec.y);
  doubleSpinBox_Bz->setValue(_biasVec.z);

  ValuesChanged();
}

void ScaleAndBiasDlg::ScaleIsotropicVol() {
  FLOATVECTOR3 s = (m_max-m_min)/(m_maxVolume-m_minVolume);
  FLOATVECTOR3 _scaleVec = 1.0f/s;

  float scale = _scaleVec.minVal();
  FLOATVECTOR3 c = ((m_max+m_min)/2.0f)*scale;
  FLOATVECTOR3 _biasVec = -c;

  doubleSpinBox_Sx->setValue(scale);
  doubleSpinBox_Sy->setValue(scale);
  doubleSpinBox_Sz->setValue(scale);

  doubleSpinBox_Bx->setValue(_biasVec.x);
  doubleSpinBox_By->setValue(_biasVec.y);
  doubleSpinBox_Bz->setValue(_biasVec.z);

  ValuesChanged();
}

void ScaleAndBiasDlg::ScaleUnisotropicVol() {
  FLOATVECTOR3 s = (m_max-m_min)/(m_maxVolume-m_minVolume);
  FLOATVECTOR3 _scaleVec = 1.0f/s;
  FLOATVECTOR3 c = ((m_max+m_min)/2.0f)*_scaleVec;
  FLOATVECTOR3 _biasVec = -c;

  doubleSpinBox_Sx->setValue(_scaleVec.x);
  doubleSpinBox_Sy->setValue(_scaleVec.y);
  doubleSpinBox_Sz->setValue(_scaleVec.z);

  doubleSpinBox_Bx->setValue(_biasVec.x);
  doubleSpinBox_By->setValue(_biasVec.y);
  doubleSpinBox_Bz->setValue(_biasVec.z);

  ValuesChanged();
}


void ScaleAndBiasDlg::ValuesChanged() {
  m_scaleVec = FLOATVECTOR3(doubleSpinBox_Sx->value(),
                          doubleSpinBox_Sy->value(),
                          doubleSpinBox_Sz->value());
  m_biasVec = FLOATVECTOR3(doubleSpinBox_Bx->value(),
                         doubleSpinBox_By->value(),
                         doubleSpinBox_Bz->value());

  UpdatePostSize();
}


void ScaleAndBiasDlg::UpdatePreSize() {
  FLOATVECTOR3 c = (m_max+m_min)/2.0f;
  FLOATVECTOR3 s = m_max-m_min;

  // pre scale size
  QString text = tr("Before scale and bias: "
                    "Size: [%1, %2, %3] "
                    "Center: [%4, %5, %6]").arg(s.x,0,'f',3).arg(s.y,0,'f',3).arg(s.z,0,'f',3)
                                           .arg(c.x,0,'f',3).arg(c.y,0,'f',3).arg(c.z,0,'f',3);
  label_currentDim->setText(text);
}

void ScaleAndBiasDlg::UpdatePostSize() {
  FLOATVECTOR3 c = (m_max+m_min)/2.0f;
  FLOATVECTOR3 s = (m_max-m_min);

  c = c*m_scaleVec + m_biasVec;
  s = s*m_scaleVec;
  
  QString text = tr("After scale and bias: "
                    "Size: [%1, %2, %3] "
                    "Center: [%4, %5, %6]").arg(s.x,0,'f',3).arg(s.y,0,'f',3).arg(s.z,0,'f',3)
                                           .arg(c.x,0,'f',3).arg(c.y,0,'f',3).arg(c.z,0,'f',3);
  label_targetDim->setText(text);

}

void ScaleAndBiasDlg::setupUi(QDialog *ScaleAndBiasDlg, const QString& strDesc) {
  Ui_ScaleAndBiasDlg::setupUi(ScaleAndBiasDlg);
  ToggleExpertView();

  UpdatePreSize();

  label_meshDesc->setText(strDesc);
  ValuesChanged();
}


void ScaleAndBiasDlg::ApplyExpertMatrix() {
  QStringList list;
  list.append(lineEdit_m11->text());
  list.append(lineEdit_m21->text());
  list.append(lineEdit_m31->text());
  list.append(lineEdit_m41->text());

  list.append(lineEdit_m12->text());
  list.append(lineEdit_m22->text());
  list.append(lineEdit_m32->text());
  list.append(lineEdit_m42->text());

  list.append(lineEdit_m13->text());
  list.append(lineEdit_m23->text());
  list.append(lineEdit_m33->text());
  list.append(lineEdit_m43->text());

  list.append(lineEdit_m14->text());
  list.append(lineEdit_m24->text());
  list.append(lineEdit_m34->text());
  list.append(lineEdit_m44->text());

  QSettings settings;
  settings.setValue("Transformation/MeshExpertMatrix", list);

  emit ApplyMatrixTransform(this);
  m_min = m_pMesh->GetMin();
  m_max = m_pMesh->GetMax();
  UpdatePreSize();
  UpdatePostSize();
}

void ScaleAndBiasDlg::CopyScaleAndBias() {
  FLOATMATRIX4 bias;
  FLOATMATRIX4 scale;

  bias.Translation(m_biasVec);
  scale.Scaling(m_scaleVec);

  SetExpertTransform(scale*bias);
}

void ScaleAndBiasDlg::ToggleExpertView() {
  groupBox_Expert->setVisible(checkBoxShowExpert->isChecked());
  pushButton_apply->setVisible(!checkBoxShowExpert->isChecked());
  retranslateUi(this);
  repaint();
  resize(QSize(0,0));
}

bool ScaleAndBiasDlg::GetApplyAll() const {
  return checkBox_applyAll->isChecked();
}

FLOATMATRIX4 ScaleAndBiasDlg::GetExpertTransform() const {
  FLOATMATRIX4 m;
  bool ok = true;

  m.m11 =lineEdit_m11->text().toFloat(&ok); if (!ok) return FLOATMATRIX4();
  m.m21 =lineEdit_m21->text().toFloat(&ok); if (!ok) return FLOATMATRIX4();
  m.m31 =lineEdit_m31->text().toFloat(&ok); if (!ok) return FLOATMATRIX4();
  m.m41 =lineEdit_m41->text().toFloat(&ok); if (!ok) return FLOATMATRIX4();

  m.m12 =lineEdit_m12->text().toFloat(&ok); if (!ok) return FLOATMATRIX4();
  m.m22 =lineEdit_m22->text().toFloat(&ok); if (!ok) return FLOATMATRIX4();
  m.m32 =lineEdit_m32->text().toFloat(&ok); if (!ok) return FLOATMATRIX4();
  m.m42 =lineEdit_m42->text().toFloat(&ok); if (!ok) return FLOATMATRIX4();

  m.m13 =lineEdit_m13->text().toFloat(&ok); if (!ok) return FLOATMATRIX4();
  m.m23 =lineEdit_m23->text().toFloat(&ok); if (!ok) return FLOATMATRIX4();
  m.m33 =lineEdit_m33->text().toFloat(&ok); if (!ok) return FLOATMATRIX4();
  m.m43 =lineEdit_m43->text().toFloat(&ok); if (!ok) return FLOATMATRIX4();

  m.m14 =lineEdit_m14->text().toFloat(&ok); if (!ok) return FLOATMATRIX4();
  m.m24 =lineEdit_m24->text().toFloat(&ok); if (!ok) return FLOATMATRIX4();
  m.m34 =lineEdit_m34->text().toFloat(&ok); if (!ok) return FLOATMATRIX4();
  m.m44 =lineEdit_m44->text().toFloat(&ok); if (!ok) return FLOATMATRIX4();

  return m;
}

void ScaleAndBiasDlg::RestoreLast() {
  QSettings settings;
  QStringList entries = settings.value("Transformation/MeshExpertMatrix").toStringList();
  if (entries.count() == 16) {
    lineEdit_m11->setText(tr("%1").arg(entries[0],0,'f'));
    lineEdit_m21->setText(tr("%1").arg(entries[1],0,'f'));
    lineEdit_m31->setText(tr("%1").arg(entries[2],0,'f'));
    lineEdit_m41->setText(tr("%1").arg(entries[3],0,'f'));

    lineEdit_m12->setText(tr("%1").arg(entries[4],0,'f'));
    lineEdit_m22->setText(tr("%1").arg(entries[5],0,'f'));
    lineEdit_m32->setText(tr("%1").arg(entries[6],0,'f'));
    lineEdit_m42->setText(tr("%1").arg(entries[7],0,'f'));

    lineEdit_m13->setText(tr("%1").arg(entries[8],0,'f'));
    lineEdit_m23->setText(tr("%1").arg(entries[9],0,'f'));
    lineEdit_m33->setText(tr("%1").arg(entries[10],0,'f'));
    lineEdit_m43->setText(tr("%1").arg(entries[11],0,'f'));

    lineEdit_m14->setText(tr("%1").arg(entries[12],0,'f'));
    lineEdit_m24->setText(tr("%1").arg(entries[13],0,'f'));
    lineEdit_m34->setText(tr("%1").arg(entries[14],0,'f'));
    lineEdit_m44->setText(tr("%1").arg(entries[15],0,'f'));
  }
}


void ScaleAndBiasDlg::InvertMatrix() {
  FLOATMATRIX4 m = GetExpertTransform();
  SetExpertTransform(m.inverse());
}

void ScaleAndBiasDlg::SetExpertTransform(const FLOATMATRIX4& m) {
  lineEdit_m11->setText(tr("%1").arg(m.m11,0,'f'));
  lineEdit_m21->setText(tr("%1").arg(m.m21,0,'f'));
  lineEdit_m31->setText(tr("%1").arg(m.m31,0,'f'));
  lineEdit_m41->setText(tr("%1").arg(m.m41,0,'f'));

  lineEdit_m12->setText(tr("%1").arg(m.m12,0,'f'));
  lineEdit_m22->setText(tr("%1").arg(m.m22,0,'f'));
  lineEdit_m32->setText(tr("%1").arg(m.m32,0,'f'));
  lineEdit_m42->setText(tr("%1").arg(m.m42,0,'f'));

  lineEdit_m13->setText(tr("%1").arg(m.m13,0,'f'));
  lineEdit_m23->setText(tr("%1").arg(m.m23,0,'f'));
  lineEdit_m33->setText(tr("%1").arg(m.m33,0,'f'));
  lineEdit_m43->setText(tr("%1").arg(m.m43,0,'f'));

  lineEdit_m14->setText(tr("%1").arg(m.m14,0,'f'));
  lineEdit_m24->setText(tr("%1").arg(m.m24,0,'f'));
  lineEdit_m34->setText(tr("%1").arg(m.m34,0,'f'));
  lineEdit_m44->setText(tr("%1").arg(m.m44,0,'f'));
}

void ScaleAndBiasDlg::Restore() {
  emit RestoreTransform(this);
  m_min = m_pMesh->GetMin();
  m_max = m_pMesh->GetMax();
  UpdatePreSize();
  UpdatePostSize();
}

void ScaleAndBiasDlg::Save() {
  emit SaveTransform(this);
}

void ScaleAndBiasDlg::Apply() {
  emit ApplyTransform(this);
  m_min = m_pMesh->GetMin();
  m_max = m_pMesh->GetMax();
  UpdatePreSize();
  UpdatePostSize();
}

