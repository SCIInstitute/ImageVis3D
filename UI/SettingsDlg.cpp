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

SettingsDlg::SettingsDlg(MasterController& MasterController, TabID eTabID /* = MEM_TAB*/, QWidget* parent /* = 0 */, Qt::WindowFlags flags /* = 0 */) : 
  QDialog(parent, flags),
  m_MasterController(MasterController),
  m_eTabID(eTabID)
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
}


UINT64 SettingsDlg::GetGPUMem() {
  return UINT64(horizontalSlider_GPUMem->value())*1024*1024;
}

UINT64 SettingsDlg::GetCPUMem() {
  return UINT64(horizontalSlider_CPUMem->value())*1024*1024;
}


void SettingsDlg::Data2Form(UINT64 iMaxCPU, UINT64 iMaxGPU) {
    horizontalSlider_CPUMem->setValue(iMaxCPU / (1024*1024));
    horizontalSlider_GPUMem->setValue(iMaxGPU / (1024*1024));
}
