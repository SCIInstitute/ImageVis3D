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

#include <cmath>
#include <QtCore/QSettings>
#include <QtCore/QTimer>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QMdiSubWindow>

#include "../Tuvok/Renderer/GPUMemMan/GPUMemMan.h"
#include "../Tuvok/DebugOut/TextfileOut.h"
#include "../Tuvok/Controller/Controller.h"

#include "ImageVis3D.h"
#include "../Tuvok/Basics/MathTools.h"
#include "../Tuvok/Basics/SystemInfo.h"
#include "../Tuvok/Basics/SysTools.h"
#include "SettingsDlg.h"
#include "BasicSettingsDlg.h"

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

void MainWindow::SaveSettings(
  uint64_t CPUMem, uint64_t maxCPU, uint64_t GPUMem,
  uint64_t maxGPU, bool ignoreMax,
  const std::wstring& tempDir,
  bool checksum, unsigned framerate, bool lowResSubframes, unsigned LODDelay,
  unsigned activeTS, unsigned inactiveTS, bool writeLog, bool showCrashDialog,
  const std::wstring& logFile, uint32_t logLevel, bool showVersion,
  bool autoSaveGeo, bool autoSaveWSP, bool autoLockCloned, bool absoluteLocks,
  bool checkForUpdates,
  bool checkForDevBuilds, bool showWelcome, bool invertWheel, bool i3mFeatures,
  unsigned volRenType, unsigned blendPrecision, bool powerTwo,
  bool downsampleTo8, bool disableBorder, const FLOATVECTOR3& backColor1,
  const FLOATVECTOR3& backColor2, const FLOATVECTOR4& textColor,
  const QString& logo, int logoPosition, unsigned maxBrickSize,
  unsigned builderBrickSize, bool medianFilter, bool clampToEdge,
  uint32_t compression, uint32_t compressionLevel, bool experimentalFeatures,
  bool advancedSettings, uint32_t layout
) {
  QSettings settings;
  settings.beginGroup("Memory"); {
    settings.setValue("MaxGPUMem", static_cast<qulonglong>(GPUMem));
    settings.setValue("MaxCPUMem", static_cast<qulonglong>(CPUMem));

    settings.setValue("OverrideDetectedMaxima", ignoreMax);
    settings.setValue("OverriddenCPUMax", static_cast<qulonglong>(maxCPU));
    settings.setValue("OverriddenGPUMax", static_cast<qulonglong>(maxGPU));
    settings.setValue("TempDir", QString::fromStdWString(tempDir));
    settings.setValue("MaxBricksize", maxBrickSize);
    settings.setValue("BuilderBrickSize", builderBrickSize);
    settings.setValue("UseMedian", medianFilter);
    settings.setValue("ClampToEdge", clampToEdge);
    settings.setValue("Compression", compression);
    settings.setValue("CompressionLevel", compressionLevel);
    settings.setValue("Layout", layout);
  } settings.endGroup();

 settings.beginGroup("Performance"); {
    settings.setValue("Quickopen", checksum);
    settings.setValue("MinFrameRate", framerate);
    settings.setValue("UseAllMeans", lowResSubframes);
    settings.setValue("LODDelay", LODDelay);
    settings.setValue("ActiveTS", activeTS);
    settings.setValue("InactiveTS", inactiveTS);
    settings.setValue("WriteLogFile", writeLog);
    settings.setValue("ShowCrashDialog", showCrashDialog);
    settings.setValue("LogFileName", QString::fromStdWString(logFile));
    settings.setValue("LogLevel", logLevel);
  } settings.endGroup();

  settings.beginGroup("UI"); {
    settings.setValue("VersionInTitle", showVersion);
    settings.setValue("AutoSaveGEO", autoSaveGeo);
    settings.setValue("AutoSaveWSP", autoSaveWSP);
    settings.setValue("AutoLockClonedWindow", autoLockCloned);
    settings.setValue("AbsoluteViewLocks", absoluteLocks);

    settings.setValue("CheckForUpdatesOnStartUp", checkForUpdates);
    settings.setValue("CheckForDevBuilds", checkForDevBuilds);
    settings.setValue("ShowWelcomeScreen", showWelcome);
    settings.setValue("InvertMouseWheel", invertWheel);
    settings.setValue("I3MFeatures", i3mFeatures);
    settings.setValue("ExperimentalFeatures", experimentalFeatures);
  } settings.endGroup();

  settings.beginGroup("Renderer"); {
    settings.setValue("RendererType", volRenType);
    settings.setValue("BlendPrecisionMode", blendPrecision);
    settings.setValue("PowerOfTwo", powerTwo);
    settings.setValue("DownSampleTo8Bits", downsampleTo8);
    settings.setValue("DisableBorder", disableBorder);
    settings.setValue("Background1R", backColor1.x);
    settings.setValue("Background1G", backColor1.y);
    settings.setValue("Background1B", backColor1.z);
    settings.setValue("Background2R", backColor2.x);
    settings.setValue("Background2G", backColor2.y);
    settings.setValue("Background2B", backColor2.z);
    settings.setValue("TextR", textColor.x);
    settings.setValue("TextG", textColor.y);
    settings.setValue("TextB", textColor.z);
    settings.setValue("TextA", textColor.w);
    settings.setValue("LogoFilename", logo);
    settings.setValue("LogoPosition", logoPosition);
  } settings.endGroup();

  ApplySettings();

  settings.setValue("AdvancedSettings", advancedSettings);
}

bool MainWindow::ShowAdvancedSettings(bool bInitializeOnly) {
    QSettings settings;
    SettingsDlg settingsDlg(m_pActiveRenderWin != NULL, m_MasterController, this);

    shared_ptr<LuaScripting> ss(m_MasterController.LuaScript());
    settings.beginGroup("Memory");
    uint64_t iMaxGPUmb = settings.value("MaxGPUMem",
      static_cast<qulonglong>(UINT64_INVALID)
    ).toULongLong();
    uint64_t iMaxCPUmb = std::min<uint64_t>(settings.value("MaxCPUMem",
      static_cast<qulonglong>(UINT64_INVALID)).toULongLong(),
      m_MasterController.SysInfo()->GetCPUMemSize()
    );
    // All GPU and CPU memory settings are in megabytes. Hack to resolve 64-bit
    // QSettings serialization issues on Mac OS X.
    // QVariant was being set correctly, but when serialized through QSettings,
    // QVariant would lose its type ulonglong (5) destroying the 4 higher order
    // bytes in the process.
    uint64_t iMaxGPU = iMaxGPUmb * 1000000;
    uint64_t iMaxCPU = iMaxCPUmb * 1000000;

    bool bOverrideDetMax = settings.value("OverrideDetectedMaxima", false).toBool();
    unsigned int iOverMaxCPU = settings.value("OverriddenCPUMax", 0).toUInt();
    unsigned int iOverMaxGPU = settings.value("OverriddenGPUMax", 0).toUInt();

    std::wstring strTempDir = settings.value("TempDir", QString::fromStdWString(m_strTempDir)).toString().toStdWString();
    unsigned int iMaxBrickSize = settings.value(
        "MaxBricksize", static_cast<qulonglong>(MathTools::Log2(
                ss->cexecRet<uint64_t>("tuvok.io.getMaxBrickSize")))).toUInt();
    unsigned int iMaxMaxBrickSize = settings.value("MaxMaxBricksize", 14).toUInt();

    // since OpenGL specs are only available if we've already opened a
    // window, this value may be invalid (0)
    if (RenderWindow::GetMax3DTexDims()) {
      iMaxMaxBrickSize = MathTools::Log2(RenderWindow::GetMax3DTexDims());
      settings.setValue("MaxMaxBricksize", iMaxMaxBrickSize);
    }
  

    unsigned int iBuilderBrickSize = settings.value("BuilderBrickSize", static_cast<qulonglong>(7)).toUInt();
    bool bUseMedian = settings.value("UseMedian", false).toBool();
    bool bClampToEdge = settings.value("ClampToEdge", false).toBool();
    uint32_t iCompression = settings.value("Compression", 1).toUInt();
    uint32_t iCompressionLevel = settings.value("CompressionLevel", 1).toUInt();
    uint32_t iLayout = settings.value("Layout", 0).toUInt();
    settings.endGroup();

    settings.beginGroup("Performance");
    bool bQuickopen = settings.value("Quickopen", m_bQuickopen).toBool();
    unsigned int iMinFramerate = settings.value("MinFrameRate", m_iMinFramerate).toUInt();
    unsigned int iLODDelay = settings.value("LODDelay", m_iLODDelay).toUInt();
    bool bRenderLowResIntermediateResults = settings.value("UseAllMeans", m_bRenderLowResIntermediateResults).toBool();
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
    settingsDlg.Data2Form(bIsDirectX10Capable, iMaxCPU, iMaxGPU,
      bOverrideDetMax, iOverMaxCPU, iOverMaxGPU, strTempDir,
      bQuickopen, iMinFramerate, bRenderLowResIntermediateResults,
      iLODDelay, iActiveTS, iInactiveTS, bWriteLogFile,
      bShowCrashDialog, strLogFileName.toStdWString(),
      iLogLevel, bShowVersionInTitle, bAutoSaveGEO,
      bAutoSaveWSP, bAutoLockClonedWindow, bAbsoluteViewLocks,
      bCheckForUpdatesOnStartUp, bCheckForDevBuilds,
      bShowWelcomeScreen, bInvWheel, bI3MFeatures, iVolRenType,
      iBlendPrecisionMode, bPowerOfTwo, bDownSampleTo8Bits,
      bDisableBorder, vBackColor1, vBackColor2, vTextColor,
      strLogoFilename, iLogoPos, iMaxBrickSize, iBuilderBrickSize,
      iMaxMaxBrickSize, bUseMedian, bClampToEdge, iCompression,
      iCompressionLevel, expFeatures, iLayout
    );

    if (bInitializeOnly || settingsDlg.exec() == QDialog::Accepted) {
      this->SaveSettings(
        // We add 500000 to round the value to the nearest megabyte.
        // If this is not done, it appears that we lose 1 MB when
        // reopening the preferences dialog after making modifications
        // to the memory allocations
        ((settingsDlg.GetCPUMem() + 500000) / 1000000),
        settingsDlg.GetMaxCPUMem(),
        ((settingsDlg.GetGPUMem() + 500000) / 1000000),
        settingsDlg.GetMaxGPUMem(),
        settingsDlg.OverrideMaxMem(),
        settingsDlg.GetTempDir(),
        settingsDlg.GetQuickopen(), settingsDlg.GetMinFramerate(),
        settingsDlg.GetUseAllMeans(), settingsDlg.GetLODDelay(),
        settingsDlg.GetActiveTS(), settingsDlg.GetInactiveTS(),
        settingsDlg.GetWriteLogFile(), settingsDlg.GetShowCrashDialog(),
        settingsDlg.GetLogFileName(), (uint32_t)settingsDlg.GetLogLevel(),
        settingsDlg.GetShowVersionInTitle(), settingsDlg.GetAutoSaveGEO(),
        settingsDlg.GetAutoSaveWSP(), settingsDlg.GetAutoLockClonedWindow(),
        settingsDlg.GetAbsoluteViewLocks(),
        settingsDlg.GetCheckForUpdatesOnStartUp(),
        settingsDlg.GetCheckForDevBuilds(), settingsDlg.GetShowWelcomeScreen(),
        settingsDlg.GetInvertWheel(), settingsDlg.GetI3MFeatures(),
        settingsDlg.GetVolrenType(), settingsDlg.GetBlendPrecisionMode(),
        settingsDlg.GetUseOnlyPowerOfTwo(), settingsDlg.GetDownSampleTo8Bits(),
        settingsDlg.GetDisableBorder(), settingsDlg.GetBackgroundColor1(),
        settingsDlg.GetBackgroundColor2(), settingsDlg.GetTextColor(),
        settingsDlg.GetLogoFilename(), settingsDlg.GetLogoPos(),
        settingsDlg.GetMaxBrickSize(), settingsDlg.GetBuilderBrickSize(),
        settingsDlg.GetMedianFilter(), settingsDlg.GetClampToEdge(),
        settingsDlg.GetCompression(), settingsDlg.GetCompressionLevel(),
        settingsDlg.GetExperimentalFeatures(),
        true, settingsDlg.GetLayout()
      );

      return true;
    } else {
      if(settingsDlg.GetUseBasicSettings()) {
        // then use the advanced setting instead.
        m_bAdvancedSettings = false;
        settings.setValue("AdvancedSettings", false);
        return ShowBasicSettings(bInitializeOnly);
      }
      return false;
    }
}

bool MainWindow::ShowBasicSettings(bool initOnly ) {
  QSettings set;
  PerformanceLevel lvl = static_cast<PerformanceLevel>(
    set.value("BasicSettingsLevel", 0).toInt()
  );
  BasicSettingsDlg bsd(m_MasterController, lvl, this);
  // to detect "i want to go to advanced mode", the dialog is actually
  // rejected---we only have two options to work with---and then a boolean must
  // be queried.
  int result = bsd.exec();
  if(result == QDialog::Rejected && bsd.GetUseAdvancedSettings()) {
    // then use the advanced setting instead.
    m_bAdvancedSettings = true;
    set.setValue("AdvancedSettings", true);
    return ShowAdvancedSettings(initOnly);
  }
  if(result == QDialog::Accepted) {
    // ... apply settings.
    MESSAGE("Applying performance level %u",
            static_cast<unsigned>(bsd.GetPerformanceLevel()));
    set.setValue("BasicSettingsLevel", int(bsd.GetPerformanceLevel()));
    const SystemInfo& sinfo = *(m_MasterController.SysInfo());
    uint64_t cpumem = sinfo.GetCPUMemSize() / (1024*1024);
    uint64_t gpumem = sinfo.GetGPUMemSize() / (1024*1024);
    std::wstring tempDir;
    if(!SysTools::GetTempDirectory(tempDir)) {
      SysTools::GetHomeDirectory(tempDir);
    }
    const bool showCrashDlg = true; // always true for 'basic' settings mode.
    const bool i3mfeatures = false; // always for 'basic' settings mode.
    const FLOATVECTOR3 black(0,0,0);
    const FLOATVECTOR3 darkblue(0,0,0.2f);
    const FLOATVECTOR3 white(1,1,1);
    switch(bsd.GetPerformanceLevel()) {
      case MAX_RESPONSIVENESS:
        this->SaveSettings(static_cast<uint64_t>(ceil(cpumem * 0.75)), cpumem,
                           static_cast<uint64_t>(ceil(gpumem * 0.40)), gpumem,
                           false, tempDir, false,
                           60, true, 1500, 150, 70, false, showCrashDlg,
                           tempDir+L"imagevis3d.log", 1, true, true, true,
                           true, false, true, false, true, true, i3mfeatures,
                           MasterController::OPENGL_2DSBVR, 0, true, false,
                           false, black, darkblue,
                           white, "", 0, 8 /* log2(256) */ ,
                           7 /* log2(128) */, true, false, 1, 1,
                           false, false, 0);
        // TODO: If some other compressor or level or layout proofs to be faster
        //       , use the fastest here!
        break;
      case MIDDLE_1:
        this->SaveSettings(static_cast<uint64_t>(ceil(cpumem * 0.77)), cpumem,
                           static_cast<uint64_t>(ceil(gpumem * 0.60)), gpumem,
                           false, tempDir, false,
                           30, true, 1500, 200, 80, false, showCrashDlg,
                           tempDir+L"imagevis3d.log", 1, true, true, true,
                           true, false, true, false, true, true, i3mfeatures,
                           MasterController::OPENGL_SBVR, 2, true, false,
                           false, black, darkblue,
                           white, "", 0, 8 /* log2(256 */,
                           7 /* log2(128) */, true, false, 1, 1,
                           false, false, 0);
        // TODO: If some other compressor or level or layout proofs to be faster
        //       , use the fastest here!
        break;
      case MAX_PERFORMANCE:
        this->SaveSettings(static_cast<uint64_t>(ceil(cpumem * 0.9)), cpumem,
                           static_cast<uint64_t>(ceil(gpumem * 0.75)), gpumem,
                           false, tempDir, false,
                           10, false, 1000, 500, 100, false, showCrashDlg,
                           tempDir+L"imagevis3d.log", 0, true, true, true,
                           true, false, true, false, true, true, i3mfeatures,
                           MasterController::OPENGL_SBVR, 2, true, false,
                           false, black, darkblue,
                           white, "", 0, 8 /* log2(256 */,
                           7 /* log2(128) */, true, false, 1, 1,
                           false, false, 0);
        // TODO: If some other compressor or level or layout proofs to be faster
        //       , use the fastest here!
        break;
    }
  }
  return true;
}

bool MainWindow::ShowSettings(bool initOnly) {
  if(m_bAdvancedSettings) {
    return ShowAdvancedSettings(initOnly);
  } else {
    return ShowBasicSettings(initOnly);
  }
}

void MainWindow::ApplySettings() {
  QSettings settings;

  shared_ptr<LuaScripting> ss(m_MasterController.LuaScript());
  // Read settings
  m_bAdvancedSettings =
    settings.value("AdvancedSettings", m_bAdvancedSettings).toBool();
  settings.beginGroup("Performance");
  m_bQuickopen     = settings.value("Quickopen", m_bQuickopen).toBool();
  m_iMinFramerate  = settings.value("MinFrameRate", m_iMinFramerate).toUInt();
  m_iLODDelay      = settings.value("LODDelay", m_iLODDelay).toUInt();
  m_bRenderLowResIntermediateResults = settings.value("UseAllMeans", m_bRenderLowResIntermediateResults).toBool();
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

  settings.endGroup();

  actionTransfer_to_ImageVis3D_Mobile_Device->setVisible(m_bI3MFeatures);

  settings.beginGroup("Renderer");
  m_eVolumeRendererType = (MasterController::EVolumeRendererType)settings.value("RendererType", (unsigned int)m_eVolumeRendererType).toUInt();
  m_iBlendPrecisionMode = settings.value("BlendPrecisionMode", m_iBlendPrecisionMode).toUInt();
  m_bPowerOfTwo = settings.value("PowerOfTwo", m_bPowerOfTwo).toBool();
  m_bDownSampleTo8Bits = settings.value("DownSampleTo8Bits", m_bDownSampleTo8Bits).toBool();
  m_bDisableBorder = settings.value("DisableBorder", m_bDisableBorder).toBool();

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
  uint64_t iMaxCPUmb = std::min<uint64_t>(settings.value("MaxCPUMem", static_cast<qulonglong>(UINT64_INVALID)).toULongLong(), m_MasterController.SysInfo()->GetCPUMemSize());
  uint64_t iMaxGPUmb = settings.value("MaxGPUMem", static_cast<qulonglong>(UINT64_INVALID)).toULongLong();
  m_strTempDir = settings.value("TempDir", QString::fromStdWString(m_strTempDir)).toString().toStdWString();
  uint64_t iMaxCPU = iMaxCPUmb * 1000000; // IEEE MB standard
  uint64_t iMaxGPU = iMaxGPUmb * 1000000;

  uint64_t iMaxBrickSizeLog = MathTools::Log2(ss->cexecRet<uint64_t>("tuvok.io.getMaxBrickSize"));
  uint64_t iMaxBrickSize = MathTools::Pow2((uint64_t)settings.value("MaxBricksize",  static_cast<qulonglong>(iMaxBrickSizeLog)).toUInt());
  iMaxBrickSizeLog = MathTools::Log2(iMaxBrickSize);

  uint64_t iBuilderBrickSize = MathTools::Pow2((uint64_t)settings.value("BuilderBrickSize", 7).toUInt());
  bool bUseMedian = settings.value("UseMedian", false).toBool();
  bool bClampToEdge = settings.value("ClampToEdge", false).toBool();
  uint32_t iCompression = settings.value("Compression", 1).toUInt();
  uint32_t iCompressionLevel = settings.value("CompressionLevel", 1).toUInt();
  uint32_t iLayout = settings.value("Layout", 0).toUInt();
  settings.endGroup();

  // sanity check: make sure a RGBA float brick would fit into the specified memory
  uint64_t _iMaxBrickSizeLog = iMaxBrickSizeLog;
  while (iMaxBrickSizeLog > 1 && iMaxBrickSize*iMaxBrickSize*iMaxBrickSize*4*sizeof(float) > iMaxCPU) {
    iMaxBrickSizeLog--;
    iMaxBrickSize = MathTools::Pow2(iMaxBrickSizeLog);
  }

  if (_iMaxBrickSizeLog != iMaxBrickSizeLog)  {
    WARNING("Reducing max bricksize from %i to %i because CPU memory limit would not allow bricks to be loaded", int(MathTools::Pow2(_iMaxBrickSizeLog)), int(MathTools::Pow2(iMaxBrickSizeLog)));
  }


  if (!ss->cexecRet<bool>("tuvok.io.setMaxBrickSize", iMaxBrickSize, iBuilderBrickSize)) {
    WARNING("Invalid MaxBrickSize read from configuration, ignoring value. Please check the configuration in the settings dialog.");
  }

  ss->cexec("tuvok.io.setUseMedianFilter", bUseMedian);
  ss->cexec("tuvok.io.setClampToEdge", bClampToEdge);
  ss->cexec("tuvok.io.setUVFCompression", iCompression);
  ss->cexec("tuvok.io.setUVFCompressionLevel", iCompressionLevel);
  ss->cexec("tuvok.io.setUVFLayout", iLayout);

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

  // as the "avoid compositing" setting may enable/disable the ability to do clearview
  // we must doublecheck the state of the controls
  if (m_pActiveRenderWin) ToggleClearViewControls(int(m_pActiveRenderWin->GetDynamicRange().second));
}

void MainWindow::ApplySettings(RenderWindow* renderWin) {
  QSettings settings;

  if (!renderWin || !renderWin->IsRendererValid()) return;

  renderWin->SetColors(m_vBackgroundColors[0],
                       m_vBackgroundColors[1], m_vTextColor);
  renderWin->SetBlendPrecision(AbstrRenderer::EBlendPrecision(m_iBlendPrecisionMode));
  renderWin->SetPerfMeasures(m_iMinFramerate, m_bRenderLowResIntermediateResults, 2.0f, 2.0f, m_iLODDelay/m_pRedrawTimer->interval(), m_iActiveTS, m_iInactiveTS);
  renderWin->SetLogoParams(m_strLogoFilename, m_iLogoPos);
  renderWin->SetAbsoluteViewLock(m_bAbsoluteViewLocks);
  renderWin->SetInvMouseWheel(m_bInvWheel);
}


void MainWindow::ToggleLogFile() {
  if ( m_pTextout && m_strLogFileName.toStdWString() != m_pTextout->GetFileName()) {
    Controller::Instance().RemoveDebugOut(m_pTextout);
    m_pTextout = NULL;
  }

  if (m_bWriteLogFile) {
    bool bNewOut = !m_pTextout;
    if (!m_pTextout)
      m_pTextout = new TextfileOut(m_strLogFileName.toStdWString());

    m_pTextout->SetShowErrors(true);
    m_pTextout->SetShowWarnings(m_iLogLevel > 0);
    m_pTextout->SetShowMessages(m_iLogLevel > 1);

    if (bNewOut) Controller::Instance().AddDebugOut(m_pTextout);
  }
}
