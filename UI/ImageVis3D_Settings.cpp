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
#include <QtGui/QMdiSubWindow>

using namespace std;

void MainWindow::CheckSettings() {
  QSettings settings;

  // if memory isn't set this must be the first time we run this app
  if (UINT64_INVALID == settings.value("Memory/MaxGPUMem", UINT64_INVALID).toULongLong()) {
    do {
      QMessageBox::information(this, "Initial Setup", "As this is the first "
                               "time you've started ImageVis3D on this system, "
                               "you need to configure the initial settings.  "
                               "In particular, the memory usage settings need "
                               "to be configured according to the hardware "
                               "configuration of the machine.  Note that "
                               "these settings can be changed later in the "
                               "settings screen.");
    } while(!ShowSettings());
  } else ApplySettings();
}

bool MainWindow::ShowSettings() {
    QSettings settings;
    SettingsDlg settingsDlg(m_MasterController, this);

    // load first setting to see if we allready saved something
    UINT64 iMaxGPU = settings.value("Memory/MaxGPUMem", UINT64_INVALID).toULongLong();

    // if memory is set load other values (otherwise the dialog box will initialize with defaults
    if (iMaxGPU != UINT64_INVALID) {
      // load other settings here
      UINT64 iMaxCPU = std::min<UINT64>(settings.value("Memory/MaxCPUMem", UINT64_INVALID).toULongLong(), m_MasterController.SysInfo()->GetCPUMemSize());
  
      settings.beginGroup("Performance");
      bool bQuickopen = settings.value("Quickopen", m_bQuickopen).toBool();
      unsigned int iMinFramerate = settings.value("MinFrameRate", m_iMinFramerate).toUInt();
      unsigned int iLODDelay = settings.value("LODDelay", m_iLODDelay).toUInt();
      unsigned int iActiveTS = settings.value("ActiveTS", m_iActiveTS).toUInt();
      unsigned int iInactiveTS = settings.value("InactiveTS", m_iInactiveTS).toUInt();
      settings.endGroup();

      settings.beginGroup("UI");
      bool bAutoSaveGEO = settings.value("AutoSaveGEO", m_bAutoSaveGEO).toBool();
      bool bAutoSaveWSP = settings.value("AutoSaveWSP", m_bAutoSaveWSP).toBool();
      bool bAutoLockClonedWindow = settings.value("AutoLockClonedWindow", m_bAutoLockClonedWindow).toBool();
      bool bAbsoluteViewLocks = settings.value("AbsoluteViewLocks", m_bAbsoluteViewLocks).toBool();
      settings.endGroup();

      settings.beginGroup("Renderer");
      unsigned int iVolRenType = settings.value("RendererType", (unsigned int)m_eVolumeRendererType).toUInt();
      unsigned int iBlendPrecisionMode = settings.value("BlendPrecisionMode", 0).toUInt();
      bool bPowerOfTwo = settings.value("PowerOfTwo", m_bPowerOfTwo).toBool();
      bool bAvoidCompositing = settings.value("AvoidCompositing", m_bAvoidCompositing).toBool();
      
      FLOATVECTOR3 vBackColor1(settings.value("Background1R", 0.0f).toULongLong(),
                              settings.value("Background1G", 0.0f).toULongLong(),
                              settings.value("Background1B", 0.0f).toULongLong());

      FLOATVECTOR3 vBackColor2(settings.value("Background2R", 0.0f).toULongLong(),
                              settings.value("Background2G", 0.0f).toULongLong(),
                              settings.value("Background2B", 0.0f).toULongLong());

      FLOATVECTOR4 vTextColor(settings.value("TextR", 1.0f).toULongLong(),
                              settings.value("TextG", 1.0f).toULongLong(),
                              settings.value("TextB", 1.0f).toULongLong(),
                              settings.value("TextA", 1.0f).toULongLong());

      QString strLogoFilename = settings.value("LogoFilename", m_strLogoFilename).toString();
      int iLogoPos            = settings.value("LogoPosition", m_iLogoPos).toInt();
      settings.endGroup();

      // hand data to form
      settingsDlg.Data2Form(iMaxCPU, iMaxGPU, 
                            bQuickopen, iMinFramerate, iLODDelay, iActiveTS, iInactiveTS, 
                            bAutoSaveGEO, bAutoSaveWSP, bAutoLockClonedWindow, bAbsoluteViewLocks,
                            iVolRenType, iBlendPrecisionMode, bPowerOfTwo, bAvoidCompositing,
                            vBackColor1, vBackColor2, vTextColor, strLogoFilename, iLogoPos);
    }

    if (settingsDlg.exec() == QDialog::Accepted) {

      // save settings
      settings.beginGroup("Memory");
      settings.setValue("MaxGPUMem", settingsDlg.GetGPUMem());
      settings.setValue("MaxCPUMem", settingsDlg.GetCPUMem());
      settings.endGroup();

      settings.beginGroup("Performance");
      settings.setValue("Quickopen", settingsDlg.GetQuickopen());
      settings.setValue("MinFrameRate", settingsDlg.GetMinFramerate());
      settings.setValue("LODDelay", settingsDlg.GetLODDelay());
      settings.setValue("ActiveTS", settingsDlg.GetActiveTS());
      settings.setValue("InactiveTS", settingsDlg.GetInactiveTS());
      settings.endGroup();

      settings.beginGroup("UI");
      settings.setValue("AutoSaveGEO", settingsDlg.GetAutoSaveGEO());
      settings.setValue("AutoSaveWSP", settingsDlg.GetAutoSaveWSP());
      settings.setValue("AutoLockClonedWindow", settingsDlg.GetAutoLockClonedWindow());
      settings.setValue("AbsoluteViewLocks", settingsDlg.GetAbsoluteViewLocks());
      settings.endGroup();

      settings.beginGroup("Renderer");
      settings.setValue("RendererType", settingsDlg.GetVolrenType());
      settings.setValue("BlendPrecisionMode", settingsDlg.GetBlendPrecisionMode());
      settings.setValue("PowerOfTwo", settingsDlg.GetUseOnlyPowerOfTwo());
      settings.setValue("AvoidCompositing", settingsDlg.GetAvoidCompositing());
      settings.setValue("Background1R", settingsDlg.GetBackgroundColor1().x);
      settings.setValue("Background1G", settingsDlg.GetBackgroundColor1().y);
      settings.setValue("Background1B", settingsDlg.GetBackgroundColor1().z);
      settings.setValue("Background2R", settingsDlg.GetBackgroundColor2().x);
      settings.setValue("Background2G", settingsDlg.GetBackgroundColor2().y);
      settings.setValue("Background2B", settingsDlg.GetBackgroundColor2().z);
      settings.setValue("TextR", settingsDlg.GetTextColor().x);
      settings.setValue("TextG", settingsDlg.GetTextColor().y);
      settings.setValue("TextB", settingsDlg.GetTextColor().z);
      settings.setValue("TextA", settingsDlg.GetTextColor().w);
      settings.setValue("LogoFilename", settingsDlg.GetLogoFilename());
      settings.setValue("LogoPosition", settingsDlg.GetLogoPos());

      settings.endGroup();

      ApplySettings();

      // as the "avoid compositing" setting may enable/disable the ability to do clearview
      // we must doublecheck the state of the controls 
      if (m_ActiveRenderWin) ToggleClearViewControls(int(m_ActiveRenderWin->GetDynamicRange().x));

      return true;
    } else return false;

}

void MainWindow::ApplySettings() {
  QSettings settings;

  // Read settings 
  settings.beginGroup("Performance");
  m_bQuickopen    = settings.value("Quickopen", m_bQuickopen).toBool();
  m_iMinFramerate = settings.value("MinFrameRate", m_iMinFramerate).toUInt();
  m_iLODDelay     = settings.value("LODDelay", m_iLODDelay).toUInt();
  m_iActiveTS     = settings.value("ActiveTS", m_iActiveTS).toUInt();
  m_iInactiveTS   = settings.value("InactiveTS", m_iInactiveTS).toUInt();
  settings.endGroup();

  settings.beginGroup("UI");
  m_bAutoSaveGEO = settings.value("AutoSaveGEO", m_bAutoSaveGEO).toBool();
  m_bAutoSaveWSP = settings.value("AutoSaveWSP", m_bAutoSaveWSP).toBool();
  m_bAutoLockClonedWindow = settings.value("AutoLockClonedWindow", m_bAutoLockClonedWindow).toBool();
  m_bAbsoluteViewLocks = settings.value("AbsoluteViewLocks", m_bAbsoluteViewLocks).toBool();
  settings.endGroup();

  settings.beginGroup("Renderer");
  m_eVolumeRendererType = (MasterController::EVolumeRendererType)settings.value("RendererType", (unsigned int)m_eVolumeRendererType).toUInt();
  m_iBlendPrecisionMode = settings.value("BlendPrecisionMode", m_iBlendPrecisionMode).toUInt();
  m_bPowerOfTwo = settings.value("PowerOfTwo", m_bPowerOfTwo).toBool();
  m_bAvoidCompositing = settings.value("AvoidCompositing", m_bAvoidCompositing).toBool();

  m_vBackgroundColors[0] = FLOATVECTOR3(settings.value("Background1R", 0.0f).toULongLong(),
                          settings.value("Background1G", 0.0f).toULongLong(),
                          settings.value("Background1B", 0.0f).toULongLong());

  m_vBackgroundColors[1] = FLOATVECTOR3(settings.value("Background2R", 0.0f).toULongLong(),
                          settings.value("Background2G", 0.0f).toULongLong(),
                          settings.value("Background2B", 0.0f).toULongLong());

  m_vTextColor = FLOATVECTOR4(settings.value("TextR", 1.0f).toULongLong(),
                          settings.value("TextG", 1.0f).toULongLong(),
                          settings.value("TextB", 1.0f).toULongLong(),
                          settings.value("TextA", 1.0f).toULongLong());
  m_strLogoFilename = settings.value("LogoFilename", m_strLogoFilename).toString();
  m_iLogoPos        = settings.value("LogoPosition", m_iLogoPos).toInt();

  settings.endGroup();

  settings.beginGroup("Memory");
  UINT64 iMaxCPU = std::min<UINT64>(settings.value("MaxCPUMem", UINT64_INVALID).toULongLong(), m_MasterController.SysInfo()->GetCPUMemSize());
  UINT64 iMaxGPU = settings.value("MaxGPUMem", UINT64_INVALID).toULongLong();
  settings.endGroup();

  // Apply window settings
  for (int i = 0;i<mdiArea->subWindowList().size();i++) {
    QWidget* w = mdiArea->subWindowList().at(i)->widget();
    ApplySettings(qobject_cast<RenderWindow*>(w));
  }

  // Apply global settings

  m_MasterController.SysInfo()->SetMaxUsableCPUMem(iMaxCPU);
  m_MasterController.SysInfo()->SetMaxUsableGPUMem(iMaxGPU);
  m_MasterController.MemMan()->MemSizesChanged();
}

void MainWindow::ApplySettings(RenderWindow* renderWin) {
  QSettings settings;

  renderWin->SetColors(m_vBackgroundColors, m_vTextColor);
  renderWin->SetBlendPrecision(AbstrRenderer::EBlendPrecision(m_iBlendPrecisionMode));
  renderWin->SetPerfMeasures(m_iMinFramerate, m_iLODDelay/10, m_iActiveTS, m_iInactiveTS);
  renderWin->SetLogoParams(m_strLogoFilename, m_iLogoPos);
  renderWin->SetAbsoluteViewLock(m_bAbsoluteViewLocks);
  renderWin->SetAvoidCompositing(m_bAvoidCompositing);
}
