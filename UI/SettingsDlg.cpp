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


//!    File   : SettingsDlg.cpp
//!    Author : Jens Krueger
//!             SCI Institute
//!             University of Utah
//!    Date   : October 2008
//
//!    Copyright (C) 2008 SCI Institute

#include "SettingsDlg.h"
#include <QtGui/QColorDialog>

SettingsDlg::SettingsDlg(MasterController& MasterController, QWidget* parent /* = 0 */, Qt::WindowFlags flags /* = 0 */) : 
  QDialog(parent, flags),
  m_MasterController(MasterController),
  m_InitialGPUMemMax(0)
{
  setupUi(this);
}

SettingsDlg::~SettingsDlg(void)
{
}

void SettingsDlg::setupUi(QDialog *SettingsDlg) {
  Ui_SettingsDlg::setupUi(SettingsDlg);

  UINT64 iMaxCPUMemSize   = m_MasterController.SysInfo()->GetCPUMemSize();
  UINT64 iMaxGPUMemSize   = m_MasterController.SysInfo()->GetGPUMemSize();
  unsigned int iProcCount = m_MasterController.SysInfo()->GetNumberOfCPUs();
  unsigned int iBitWith   = m_MasterController.SysInfo()->GetProgrammBitWith();

  // init stats labels
  QString desc;
  if (iMaxCPUMemSize==0) 
    desc = tr("CPU Mem: unchecked");
  else 
    desc = tr("CPU Mem: %1 MB (%2 bytes)").arg(iMaxCPUMemSize/(1024*1024)).arg(iMaxCPUMemSize);
  label_CPUMem->setText(desc);

  if (iMaxGPUMemSize==0) 
    desc = tr("GPU Mem: unchecked");
  else 
    desc = tr("GPU Mem: %1 MB (%2 bytes)").arg(iMaxGPUMemSize/(1024*1024)).arg(iMaxGPUMemSize);
  label_GPUMem->setText(desc);

  if (iProcCount==0) 
    desc = tr("Processors: unchecked");
  else 
    desc = tr("Processors %1").arg(iProcCount);    
  label_NumProc->setText(desc);

  desc = tr("Running in %1 bit mode").arg(iBitWith);
  label_NumBits->setText(desc);

  // init mem sliders

  horizontalSlider_GPUMem->setMinimum(64);
  horizontalSlider_CPUMem->setMinimum(512);

  if (iMaxCPUMemSize == 0) {
    iMaxCPUMemSize = 32*1024;
    horizontalSlider_CPUMem->setMaximum(iMaxCPUMemSize);
    horizontalSlider_CPUMem->setValue(2*1024);
  } else {
    iMaxCPUMemSize /= 1024*1024;
    horizontalSlider_CPUMem->setMaximum(iMaxCPUMemSize);
    horizontalSlider_CPUMem->setValue(iMaxCPUMemSize*0.8f);
  }

  if (iMaxGPUMemSize == 0) {
    iMaxGPUMemSize = 4*1024;
    horizontalSlider_GPUMem->setMaximum(iMaxGPUMemSize);
    horizontalSlider_GPUMem->setValue(512);
  } else {
    iMaxGPUMemSize /= 1024*1024;
    horizontalSlider_GPUMem->setMaximum(iMaxGPUMemSize);
    horizontalSlider_GPUMem->setValue(iMaxGPUMemSize*0.8f);
  }

  m_InitialGPUMemMax = horizontalSlider_GPUMem->maximum();
}


UINT64 SettingsDlg::GetGPUMem() {
  return UINT64(horizontalSlider_GPUMem->value())*1024*1024;
}

UINT64 SettingsDlg::GetCPUMem() {
  return UINT64(horizontalSlider_CPUMem->value())*1024*1024;
}

bool SettingsDlg::GetQuickopen() {
  return checkBoxQuickload->checkState() == Qt::Checked;
}

unsigned int SettingsDlg::GetMinFramerate() {
  return (unsigned int)(horizontalSlider_MinFramerate->value());
}

unsigned int SettingsDlg::GetLODDelay() {
  return (unsigned int)(horizontalSlider_LODDelay->value());
}

unsigned int SettingsDlg::GetActiveTS() {
  return (unsigned int)(horizontalSlider_ActTS->value());
}

unsigned int SettingsDlg::GetInactiveTS() {
  return (unsigned int)(horizontalSlider_InactTS->value());
}

void SettingsDlg::Data2Form(UINT64 iMaxCPU, UINT64 iMaxGPU, 
                            bool bQuickopen, unsigned int iMinFramerate, unsigned int iLODDelay, unsigned int iActiveTS, unsigned int iInactiveTS, 
                            unsigned int iBlendPrecision, const FLOATVECTOR3& vBackColor1, const FLOATVECTOR3& vBackColor2, const FLOATVECTOR4& vTextColor) {
    horizontalSlider_CPUMem->setValue(iMaxCPU / (1024*1024));
    horizontalSlider_GPUMem->setValue(iMaxGPU / (1024*1024));

    checkBoxQuickload->setChecked(bQuickopen);
    horizontalSlider_MinFramerate->setValue(iMinFramerate);
    horizontalSlider_LODDelay->setValue(iLODDelay);
    horizontalSlider_ActTS->setValue(iActiveTS);
    horizontalSlider_InactTS->setValue(iInactiveTS);
    
    m_cBackColor1 = QColor(int(vBackColor1.x*255), int(vBackColor1.y*255),int(vBackColor1.z*255));
    m_cBackColor2 = QColor(int(vBackColor2.x*255), int(vBackColor2.y*255),int(vBackColor2.z*255));
    m_cTextColor  = QColor(int(vTextColor.x*255), int(vTextColor.y*255),int(vTextColor.z*255),int(vTextColor.w*255));

    switch (iBlendPrecision) {
      case 2    : radioButton_Prec32Bit->setChecked(true); break;
      case 1    : radioButton_Prec16Bit->setChecked(true); break;
      default   : radioButton_Prec8Bit->setChecked(true); break;
    }
    
    QString strStyle =
    tr("QPushButton { background: rgb(%1, %2, %3); color: rgb(%4, %5, %6) }").arg(m_cBackColor1.red())
                                                                             .arg(m_cBackColor1.green())
                                                                             .arg(m_cBackColor1.blue())
                                                                             .arg(255-m_cBackColor1.red())
                                                                             .arg(255-m_cBackColor1.green())
                                                                             .arg(255-m_cBackColor1.blue());

    pushButtonSelBack1->setStyleSheet( strStyle );

    strStyle =
    tr("QPushButton { background: rgb(%1, %2, %3); color: rgb(%4, %5, %6) }").arg(m_cBackColor2.red())
                                                                             .arg(m_cBackColor2.green())
                                                                             .arg(m_cBackColor2.blue())
                                                                             .arg(255-m_cBackColor2.red())
                                                                             .arg(255-m_cBackColor2.green())
                                                                             .arg(255-m_cBackColor2.blue());

    pushButtonSelBack2->setStyleSheet( strStyle );

    strStyle =
    tr("QPushButton { background: rgb(%1, %2, %3); color: rgb(%4, %5, %6) }").arg(m_cTextColor.red())
                                                                             .arg(m_cTextColor.green())
                                                                             .arg(m_cTextColor.blue())
                                                                             .arg(255-m_cTextColor.red())
                                                                             .arg(255-m_cTextColor.green())
                                                                             .arg(255-m_cTextColor.blue());

    pushButtonSelText->setStyleSheet( strStyle );

}


FLOATVECTOR3  SettingsDlg::GetBackgroundColor1() {
  return FLOATVECTOR3(m_cBackColor1.red()/255.0f,
                      m_cBackColor1.green()/255.0f,
                      m_cBackColor1.blue()/255.0f);
}


FLOATVECTOR3  SettingsDlg::GetBackgroundColor2() {
  return FLOATVECTOR3(m_cBackColor2.red()/255.0f,
                      m_cBackColor2.green()/255.0f,
                      m_cBackColor2.blue()/255.0f);
}

FLOATVECTOR4  SettingsDlg::GetTextColor() {
  return FLOATVECTOR4(m_cTextColor.red()/255.0f,
                      m_cTextColor.green()/255.0f,
                      m_cTextColor.blue()/255.0f,
                      m_cTextColor.alpha()/255.0f);
}

unsigned int SettingsDlg::GetBlendPrecisionMode() {
  if (radioButton_Prec32Bit->isChecked()) return 2; else
    if (radioButton_Prec16Bit->isChecked()) return 1; else
      return 0;
}

void SettingsDlg::SelectTextColor() {
  QColor color = QColorDialog::getColor(m_cTextColor, this);
  if (color.isValid()) {
    m_cTextColor = color;
    QString strStyle =
    tr("QPushButton { background: rgb(%1, %2, %3); color: rgb(%4, %5, %6) }").arg(m_cTextColor.red())
                                                                             .arg(m_cTextColor.green())
                                                                             .arg(m_cTextColor.blue())
                                                                             .arg(255-m_cTextColor.red())
                                                                             .arg(255-m_cTextColor.green())
                                                                             .arg(255-m_cTextColor.blue());

    pushButtonSelText->setStyleSheet( strStyle );
  }  
}

void SettingsDlg::SetTextOpacity(int iOpacity) {
  m_cTextColor.setAlpha(iOpacity);
}

void SettingsDlg::SelectBackColor1() {
  QColor color = QColorDialog::getColor(m_cBackColor1, this);
  if (color.isValid()) {
    m_cBackColor1 = color;
    QString strStyle =
    tr("QPushButton { background: rgb(%1, %2, %3); color: rgb(%4, %5, %6) }").arg(m_cBackColor1.red())
                                                                             .arg(m_cBackColor1.green())
                                                                             .arg(m_cBackColor1.blue())
                                                                             .arg(255-m_cBackColor1.red())
                                                                             .arg(255-m_cBackColor1.green())
                                                                             .arg(255-m_cBackColor1.blue());

    pushButtonSelBack1->setStyleSheet( strStyle );
  }  
}

void SettingsDlg::SelectBackColor2() {
  QColor color = QColorDialog::getColor(m_cBackColor2, this);
  if (color.isValid()) {
    m_cBackColor2 = color;
    QString strStyle =
    tr("QPushButton { background: rgb(%1, %2, %3); color: rgb(%4, %5, %6) }").arg(m_cBackColor2.red())
                                                                             .arg(m_cBackColor2.green())
                                                                             .arg(m_cBackColor2.blue())
                                                                             .arg(255-m_cBackColor2.red())
                                                                             .arg(255-m_cBackColor2.green())
                                                                             .arg(255-m_cBackColor2.blue());

    pushButtonSelBack2->setStyleSheet( strStyle );
  } 
}

// make sure the user cannot select more GPU than CPU mem
void SettingsDlg::SetMaxMemCheck() {
  if (horizontalSlider_GPUMem->value() > horizontalSlider_CPUMem->value()) {
    horizontalSlider_GPUMem->setValue(horizontalSlider_CPUMem->value());
  }

  horizontalSlider_GPUMem->setMaximum(std::min<int> (horizontalSlider_CPUMem->value(),m_InitialGPUMemMax)  );
}

void SettingsDlg::LODDelayChanged() {
  QString text= tr("%1 ms").arg(horizontalSlider_LODDelay->value());
  label_LODDelayDisplay->setText(text);
}

void SettingsDlg::MinFramerateChanged() {
  QString text= tr("%1 fps").arg(horizontalSlider_MinFramerate->value());
  label_MinFrameRateDisplay->setText(text);
}

void SettingsDlg::ActTSChanged() {
  QString text= tr("%1 ms").arg(horizontalSlider_ActTS->value());
  label_ActTSDisplay->setText(text);
}

void SettingsDlg::InactTSChanged() {
  QString text= tr("%1 ms").arg(horizontalSlider_InactTS->value());
  label_InactTSDisplay->setText(text);
}
