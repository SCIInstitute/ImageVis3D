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

#include <QtCore/QSettings>
#include <QtCore/QTimer>
#include <QtGui/QMessageBox>
#include <QtGui/QMdiSubWindow>

#include "../Tuvok/Renderer/GPUMemMan/GPUMemMan.h"
#include "../Tuvok/DebugOut/TextfileOut.h"
#include "../Tuvok/Controller/Controller.h"

#include "ImageVis3D.h"
#include "../Tuvok/IO/IOManager.h"
#include "../Tuvok/Basics/SysTools.h"
#include "../Tuvok/Basics/MathTools.h"
#include "../Tuvok/Basics/SystemInfo.h"

using namespace std;

void MainWindow::CheckSettings() {
  QSettings settings;

  // if memory isn't set this must be the first time we run this app

  if (UINT64_INVALID == settings.value("Memory/MaxGPUMem",
                                       static_cast<qulonglong>(UINT64_INVALID)).toULongLong()) {
      ShowSettings(!m_bScriptMode &&
                    QMessageBox::No == QMessageBox::question(this, "Initial Setup",
                             "As this is the first "
                             "time you've started ImageVis3D on this system, "
                             "ImageVis3D has been configured with the default "
                             "parameters. You may want to verify this inital "
                             "configuration. In particular, the memory usage "
                             "settings need to be configured according to the "
                             "hardware configuration of the machine. Do you want "
                             "to check the settings now?", QMessageBox::Yes, QMessageBox::No));
  }
  ApplySettings();

  // Apply startup settings

  E2DTransferFunctionMode tfMode = (E2DTransferFunctionMode)(settings.value("UI/2DTFMode", int(TFM_BASIC)).toInt());

  if (tfMode != m_2DTransferFunction->Get2DTFMode()) {
    Transfer2DToggleTFMode();
  }
}

bool MainWindow::ShowSettings(bool bInitializeOnly) {
    QSettings settings;
    SettingsDlg settingsDlg(m_pActiveRenderWin != NULL, m_MasterController, this);

    settings.beginGroup("Memory");
    UINT64 iMaxGPU = settings.value("MaxGPUMem", static_cast<qulonglong>(UINT64_INVALID)).toULongLong();
    UINT64 iMaxCPU = std::min<UINT64>(settings.value("MaxCPUMem", static_cast<qulonglong>(UINT64_INVALID)).toULongLong(), m_MasterController.SysInfo()->GetCPUMemSize());

    bool bOverrideDetMax = settings.value("OverrideDetectedMaxima", false).toBool();
    unsigned int iOverMaxCPU = settings.value("OverriddenCPUMax", 0).toUInt();
    unsigned int iOverMaxGPU = settings.value("OverriddenGPUMax", 0).toUInt();

    string strTempDir(settings.value("TempDir", m_strTempDir.c_str()).toString().toAscii());
    unsigned int iMaxBrickSize = settings.value("MaxBricksize", static_cast<qulonglong>(MathTools::Log2(m_MasterController.IOMan()->GetMaxBrickSize()))).toUInt();
    unsigned int iMaxMaxBrickSize = settings.value("MaxMaxBricksize", 14).toUInt();
    
    if (RenderWindow::GetMax3DTexDims()) { // as OpenGL specs are only available if we already opened a window this valu may be invalid (0)
      iMaxMaxBrickSize = MathTools::Log2(RenderWindow::GetMax3DTexDims());
      settings.setValue("MaxMaxBricksize", iMaxMaxBrickSize);
    }
  

    settings.endGroup();
    

    settings.beginGroup("Performance");
    bool bQuickopen = settings.value("Quickopen", m_bQuickopen).toBool();
    unsigned int iMinFramerate = settings.value("MinFrameRate", m_iMinFramerate).toUInt();
    unsigned int iLODDelay = settings.value("LODDelay", m_iLODDelay).toUInt();
    bool bUseAllMeans = settings.value("UseAllMeans", m_bUseAllMeans).toBool();
    unsigned int iActiveTS = settings.value("ActiveTS", m_iActiveTS).toUInt();
    unsigned int iInactiveTS = settings.value("InactiveTS", m_iInactiveTS).toUInt();
    bool bWriteLogFile = settings.value("WriteLogFile", m_bWriteLogFile).toBool();
    bool bShowCrashDialog = settings.value("ShowCrashDialog", true).toBool();
    QString strLogFileName = settings.value("LogFileName", m_strLogFileName).toString();
    unsigned int iLogLevel = settings.value("LogLevel", m_iLogLevel).toUInt();
    settings.endGroup();

    settings.beginGroup("UI");
    bool bShowVersionInTitle = settings.value("VersionInTitle", m_bShowVersionInTitle).toBool();
    bool bAutoSaveGEO = settings.value("AutoSaveGEO", m_bAutoSaveGEO).toBool();
    bool bAutoSaveWSP = settings.value("AutoSaveWSP", m_bAutoSaveWSP).toBool();
    bool bAutoLockClonedWindow = settings.value("AutoLockClonedWindow", m_bAutoLockClonedWindow).toBool();
    bool bAbsoluteViewLocks = settings.value("AbsoluteViewLocks", m_bAbsoluteViewLocks).toBool();
    bool bCheckForUpdatesOnStartUp = settings.value("CheckForUpdatesOnStartUp", m_bCheckForUpdatesOnStartUp).toBool();
    bool bCheckForDevBuilds = settings.value("CheckForDevBuilds", m_bCheckForDevBuilds).toBool();
    bool bShowWelcomeScreen = settings.value("ShowWelcomeScreen", m_bShowWelcomeScreen).toBool();
    bool bInvWheel = settings.value("InvertMouseWheel", m_bInvWheel).toBool();
    bool bI3MFeatures = settings.value("I3MFeatures", m_bI3MFeatures).toBool();
    bool expFeatures = settings.value(
                        "ExperimentalFeatures",
                        m_MasterController.ExperimentalFeatures()
                       ).toBool();
    settings.endGroup();

    settings.beginGroup("Renderer");
    unsigned int iVolRenType = settings.value("RendererType", (unsigned int)m_eVolumeRendererType).toUInt();
    unsigned int iBlendPrecisionMode = settings.value("BlendPrecisionMode", 0).toUInt();
    bool bPowerOfTwo = settings.value("PowerOfTwo", m_bPowerOfTwo).toBool();
    bool bDownSampleTo8Bits = settings.value("DownSampleTo8Bits", m_bDownSampleTo8Bits).toBool();
    bool bDisableBorder = settings.value("DisableBorder", m_bDisableBorder).toBool();
    bool bNoRCClipplanes = settings.value("NoRCClipplanes", m_bNoRCClipplanes).toBool();

    FLOATVECTOR3 vBackColor1(settings.value("Background1R", 0.0).toDouble(),
                            settings.value("Background1G", 0.0).toDouble(),
                            settings.value("Background1B", 0.0).toDouble());

    FLOATVECTOR3 vBackColor2(settings.value("Background2R", 0.0).toDouble(),
                            settings.value("Background2G", 0.0).toDouble(),
                            settings.value("Background2B", 0.0).toDouble());

    FLOATVECTOR4 vTextColor(settings.value("TextR", 1.0).toDouble(),
                            settings.value("TextG", 1.0).toDouble(),
                            settings.value("TextB", 1.0).toDouble(),
                            settings.value("TextA", 1.0).toDouble());

    QString strLogoFilename = settings.value("LogoFilename", m_strLogoFilename).toString();
    int iLogoPos            = settings.value("LogoPosition", m_iLogoPos).toInt();
    settings.endGroup();

    bool bIsDirectX10Capable = m_MasterController.SysInfo()->IsDirectX10Capable();

    // hand data to form
    settingsDlg.Data2Form(bIsDirectX10Capable, iMaxCPU, iMaxGPU, bOverrideDetMax,
                          iOverMaxCPU, iOverMaxGPU, strTempDir,
                          bQuickopen, iMinFramerate, bUseAllMeans, iLODDelay, iActiveTS, iInactiveTS,
                          bWriteLogFile, bShowCrashDialog, string(strLogFileName.toAscii()), iLogLevel,
                          bShowVersionInTitle,
                          bAutoSaveGEO, bAutoSaveWSP, bAutoLockClonedWindow, bAbsoluteViewLocks,
                          bCheckForUpdatesOnStartUp, bCheckForDevBuilds, bShowWelcomeScreen,
                          bInvWheel, bI3MFeatures,
                          iVolRenType, iBlendPrecisionMode, bPowerOfTwo, bDownSampleTo8Bits,
                          bDisableBorder, bNoRCClipplanes,
                          vBackColor1, vBackColor2, vTextColor, strLogoFilename, iLogoPos,
                          iMaxBrickSize, iMaxMaxBrickSize, expFeatures);

    if (bInitializeOnly || settingsDlg.exec() == QDialog::Accepted) {
      // save settings
      settings.beginGroup("Memory");
      settings.setValue("MaxGPUMem", static_cast<qulonglong>(settingsDlg.GetGPUMem()));
      settings.setValue("MaxCPUMem", static_cast<qulonglong>(settingsDlg.GetCPUMem()));

      settings.setValue("OverrideDetectedMaxima", settingsDlg.OverrideMaxMem());
      settings.setValue("OverriddenCPUMax", settingsDlg.GetMaxCPUMem());
      settings.setValue("OverriddenGPUMax", settingsDlg.GetMaxGPUMem());


      settings.setValue("TempDir", settingsDlg.GetTempDir().c_str());
      settings.setValue("MaxBricksize", static_cast<qulonglong>(settingsDlg.GetMaxBrickSize()));
      settings.endGroup();

      settings.beginGroup("Performance");
      settings.setValue("Quickopen", settingsDlg.GetQuickopen());
      settings.setValue("MinFrameRate", settingsDlg.GetMinFramerate());
      settings.setValue("UseAllMeans", settingsDlg.GetUseAllMeans());
      settings.setValue("LODDelay", settingsDlg.GetLODDelay());
      settings.setValue("ActiveTS", settingsDlg.GetActiveTS());
      settings.setValue("InactiveTS", settingsDlg.GetInactiveTS());
      settings.setValue("WriteLogFile", settingsDlg.GetWriteLogFile());
      settings.setValue("ShowCrashDialog", settingsDlg.GetShowCrashDialog());
      settings.setValue("LogFileName", settingsDlg.GetLogFileName().c_str());
      settings.setValue("LogLevel", settingsDlg.GetLogLevel());
      settings.endGroup();

      settings.beginGroup("UI");
      settings.setValue("VersionInTitle", settingsDlg.GetShowVersionInTitle());
      settings.setValue("AutoSaveGEO", settingsDlg.GetAutoSaveGEO());
      settings.setValue("AutoSaveWSP", settingsDlg.GetAutoSaveWSP());
      settings.setValue("AutoLockClonedWindow", settingsDlg.GetAutoLockClonedWindow());
      settings.setValue("AbsoluteViewLocks", settingsDlg.GetAbsoluteViewLocks());

      settings.setValue("CheckForUpdatesOnStartUp", settingsDlg.GetCheckForUpdatesOnStartUp());
      settings.setValue("CheckForDevBuilds", settingsDlg.GetCheckForDevBuilds());
      settings.setValue("ShowWelcomeScreen", settingsDlg.GetShowWelcomeScreen());
      settings.setValue("InvertMouseWheel", settingsDlg.GetInvertWheel());
      settings.setValue("I3MFeatures", settingsDlg.GetI3MFeatures());
      settings.setValue("ExperimentalFeatures",
                        settingsDlg.GetExperimentalFeatures());
      settings.endGroup();

      settings.beginGroup("Renderer");
      settings.setValue("RendererType", settingsDlg.GetVolrenType());
      settings.setValue("BlendPrecisionMode", settingsDlg.GetBlendPrecisionMode());
      settings.setValue("PowerOfTwo", settingsDlg.GetUseOnlyPowerOfTwo());
      settings.setValue("DownSampleTo8Bits", settingsDlg.GetDownSampleTo8Bits());
      settings.setValue("DisableBorder", settingsDlg.GetDisableBorder());
      settings.setValue("NoRCClipplanes", settingsDlg.GetNoRCClipplanes());
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
      if (m_pActiveRenderWin) ToggleClearViewControls(int(m_pActiveRenderWin->GetDynamicRange().second)-1);

      return true;
    } else return false;

}

void MainWindow::ApplySettings() {
  QSettings settings;

  // Read settings
  settings.beginGroup("Performance");
  m_bQuickopen     = settings.value("Quickopen", m_bQuickopen).toBool();
  m_iMinFramerate  = settings.value("MinFrameRate", m_iMinFramerate).toUInt();
  m_iLODDelay      = settings.value("LODDelay", m_iLODDelay).toUInt();
  m_bUseAllMeans   = settings.value("UseAllMeans", m_bUseAllMeans).toBool();
  m_iActiveTS      = settings.value("ActiveTS", m_iActiveTS).toUInt();
  m_iInactiveTS    = settings.value("InactiveTS", m_iInactiveTS).toUInt();
  m_bWriteLogFile  = settings.value("WriteLogFile", m_bWriteLogFile).toBool();
  m_strLogFileName = settings.value("LogFileName", m_strLogFileName).toString();
  m_iLogLevel      = settings.value("LogLevel", m_iLogLevel).toUInt();
  settings.endGroup();

  settings.beginGroup("UI");
  m_bShowVersionInTitle = settings.value("VersionInTitle", m_bShowVersionInTitle).toBool();
  SetTitle();
  m_bAutoSaveGEO = settings.value("AutoSaveGEO", m_bAutoSaveGEO).toBool();
  m_bAutoSaveWSP = settings.value("AutoSaveWSP", m_bAutoSaveWSP).toBool();
  m_bAutoLockClonedWindow = settings.value("AutoLockClonedWindow", m_bAutoLockClonedWindow).toBool();
  m_bAbsoluteViewLocks = settings.value("AbsoluteViewLocks", m_bAbsoluteViewLocks).toBool();
  m_bCheckForUpdatesOnStartUp = settings.value("CheckForUpdatesOnStartUp", m_bCheckForUpdatesOnStartUp).toBool();
  m_bCheckForDevBuilds = settings.value("CheckForDevBuilds", m_bCheckForDevBuilds).toBool();
  m_bShowWelcomeScreen = settings.value("ShowWelcomeScreen", m_bShowWelcomeScreen).toBool();
  m_bInvWheel = settings.value("InvertMouseWheel", m_bInvWheel).toBool();
  m_bI3MFeatures = settings.value("I3MFeatures", m_bI3MFeatures).toBool();
  bool experimental = settings.value("ExperimentalFeatures",
                                     m_MasterController.ExperimentalFeatures()).toBool();
  m_MasterController.ExperimentalFeatures(experimental);


  toolButton_CropData->setVisible(experimental);


  settings.endGroup();

  actionTransfer_to_ImageVis3D_Mobile_Device->setVisible(m_bI3MFeatures);

  settings.beginGroup("Renderer");
  m_eVolumeRendererType = (MasterController::EVolumeRendererType)settings.value("RendererType", (unsigned int)m_eVolumeRendererType).toUInt();
  m_iBlendPrecisionMode = settings.value("BlendPrecisionMode", m_iBlendPrecisionMode).toUInt();
  m_bPowerOfTwo = settings.value("PowerOfTwo", m_bPowerOfTwo).toBool();
  m_bDownSampleTo8Bits = settings.value("DownSampleTo8Bits", m_bDownSampleTo8Bits).toBool();
  m_bDisableBorder = settings.value("DisableBorder", m_bDisableBorder).toBool();
  m_bNoRCClipplanes = settings.value("NoRCClipplanes", m_bNoRCClipplanes).toBool();

  m_vBackgroundColors[0] = FLOATVECTOR3(
    settings.value("Background1R", 0.0).toDouble(),
    settings.value("Background1G", 0.0).toDouble(),
    settings.value("Background1B", 0.0).toDouble()
  );

  m_vBackgroundColors[1] = FLOATVECTOR3(
    settings.value("Background2R", 0.0).toDouble(),
    settings.value("Background2G", 0.0).toDouble(),
    settings.value("Background2B", 0.0).toDouble()
  );

  m_vTextColor = FLOATVECTOR4(
    settings.value("TextR", 1.0).toDouble(),
    settings.value("TextG", 1.0).toDouble(),
    settings.value("TextB", 1.0).toDouble(),
    settings.value("TextA", 1.0).toDouble()
  );
  m_strLogoFilename = settings.value("LogoFilename", m_strLogoFilename).toString();
  m_iLogoPos        = settings.value("LogoPosition", m_iLogoPos).toInt();

  settings.endGroup();

  settings.beginGroup("Memory");
  UINT64 iMaxCPU = std::min<UINT64>(settings.value("MaxCPUMem", static_cast<qulonglong>(UINT64_INVALID)).toULongLong(), m_MasterController.SysInfo()->GetCPUMemSize());
  UINT64 iMaxGPU = settings.value("MaxGPUMem", static_cast<qulonglong>(UINT64_INVALID)).toULongLong();
  m_strTempDir = std::string(settings.value("TempDir", m_strTempDir.c_str()).toString().toAscii());

  if (!m_MasterController.IOMan()->SetMaxBrickSize(MathTools::Pow2(settings.value("MaxBrickSize",  static_cast<qulonglong>(MathTools::Log2(m_MasterController.IOMan()->GetMaxBrickSize()))).toUInt()))) {
    WARNING("Invalid MaxBrickSize read from configuration, ignoring value. Please check the configuration in the settings dialog.");
  }
  settings.endGroup();

  // Apply window settings
  for (int i = 0;i<mdiArea->subWindowList().size();i++) {
    QWidget* w = mdiArea->subWindowList().at(i)->widget();
    ApplySettings(WidgetToRenderWin(w));
  }

  // Apply global settings
  m_MasterController.SysInfo()->SetMaxUsableCPUMem(iMaxCPU);
  m_MasterController.SysInfo()->SetMaxUsableGPUMem(iMaxGPU);
  m_MasterController.MemMan()->MemSizesChanged();
  ToggleLogFile();
}

void MainWindow::ApplySettings(RenderWindow* renderWin) {
  QSettings settings;

  if (!renderWin || !renderWin->GetRenderer()) return;

  renderWin->SetColors(m_vBackgroundColors, m_vTextColor);
  renderWin->SetBlendPrecision(AbstrRenderer::EBlendPrecision(m_iBlendPrecisionMode));
  renderWin->SetPerfMeasures(m_iMinFramerate, m_bUseAllMeans, 2.0f, 2.0f, m_iLODDelay/m_pRedrawTimer->interval(), m_iActiveTS, m_iInactiveTS);
  renderWin->SetLogoParams(m_strLogoFilename, m_iLogoPos);
  renderWin->SetAbsoluteViewLock(m_bAbsoluteViewLocks);
  renderWin->SetInvMouseWheel(m_bInvWheel);
}


void MainWindow::ToggleLogFile() {
  if ( m_pTextout && string(m_strLogFileName.toAscii()) != m_pTextout->GetFileName()) {
    Controller::Instance().RemoveDebugOut(m_pTextout);
    m_pTextout = NULL;
  }

  if (m_bWriteLogFile) {
    bool bNewOut = !m_pTextout;
    if (!m_pTextout)
      m_pTextout = new TextfileOut(string(m_strLogFileName.toAscii()));

    m_pTextout->SetShowErrors(true);
    m_pTextout->SetShowWarnings(m_iLogLevel > 0);
    m_pTextout->SetShowMessages(m_iLogLevel > 1);

    if (bNewOut) Controller::Instance().AddDebugOut(m_pTextout);
  }
}
