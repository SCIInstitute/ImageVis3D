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


//!    File   : ImageVis3D_Settings.cpp
//!    Author : Jens Krueger
//!             SCI Institute
//!             University of Utah
//!    Date   : September 2008
//
//!    Copyright (C) 2008 SCI Institute

#include "ImageVis3D.h"
#include <Basics/SysTools.h>
#include <QtCore/QSettings>
#include <QtGui/QMessageBox>

using namespace std;

void MainWindow::CheckSettings() {
  QSettings settings;

  // if memory isn't set this must be the first time we run this app
  if (UINT64_INVALID == settings.value("Memory/MaxGPUMem", UINT64_INVALID).toLongLong()) {
    do {
      QMessageBox::information(this, "Initial Setup", "As this is the first time you start ImageVis3D on this system you need to check the settings. In particular the memory usage settings need to be set according to the actual hardware configuration of thy machine. Note that these settings can also be changed later in the settings screen.");
    } while(!ShowSettings(SettingsDlg::MEM_TAB));
  } else ApplySettings();
}

bool MainWindow::ShowSettings(SettingsDlg::TabID eTabID) {
    QSettings settings;
    SettingsDlg settingsDlg(m_MasterController, eTabID, this);

    // load settings
    UINT64 iMaxGPU = settings.value("Memory/MaxGPUMem", UINT64_INVALID).toLongLong();

    // if memory is set load other values (otherwise the dialog box will initialize with defaults
    if (iMaxGPU != UINT64_INVALID) {      
      // load other settings here
      UINT64 iMaxCPU = settings.value("Memory/MaxCPUMem", UINT64_INVALID).toLongLong();     

      // hand data to form
      settingsDlg.Data2Form(iMaxCPU, iMaxGPU);
    }

    if (settingsDlg.exec() == QDialog::Accepted) {

      // save settings
      settings.beginGroup("Memory");
      settings.setValue("MaxGPUMem", settingsDlg.GetGPUMem());
      settings.setValue("MaxCPUMem", settingsDlg.GetCPUMem());
      settings.endGroup();

      // TODO save other settings here


      ApplySettings();
      return true;
    } else return false;

}


void MainWindow::ApplySettings() {
    QSettings settings;

    settings.beginGroup("Memory");
    UINT64 iMaxCPU = settings.value("MaxCPUMem", UINT64_INVALID).toLongLong();
    UINT64 iMaxGPU = settings.value("MaxGPUMem", UINT64_INVALID).toLongLong();
    settings.endGroup();

    m_MasterController.SysInfo()->SetMaxUsableCPUMem(iMaxCPU);
    m_MasterController.SysInfo()->SetMaxUsableGPUMem(iMaxGPU);
    m_MasterController.MemMan()->MemSizesChanged();

}