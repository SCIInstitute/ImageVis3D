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


//!    File   : SettingsDlg.h
//!    Author : Jens Krueger
//!             SCI Institute
//!             University of Utah
//!    Date   : July 2008
//
//!    Copyright (C) 2008 SCI Institute

#ifndef SETTINGSDLG_H
#define SETTINGSDLG_H

#include "../Tuvok/Basics/Vectors.h"
#include "../Tuvok/Controller/MasterController.h"
#include "UI/AutoGen/ui_SettingsDlg.h"

using namespace tuvok;

class SettingsDlg : public QDialog, protected Ui_SettingsDlg
{
  Q_OBJECT
  public:
    SettingsDlg(bool bWarnAPIChange,
                MasterController& masterController,
                QWidget* parent = 0,
                Qt::WindowFlags flags = 0);
    virtual ~SettingsDlg();

    uint64_t        GetGPUMem() const;
    uint64_t        GetCPUMem() const;

    bool OverrideMaxMem() const;
    unsigned int GetMaxGPUMem() const;
    unsigned int GetMaxCPUMem() const;

    std::wstring  GetTempDir() const;

    bool          GetQuickopen() const;
    unsigned int  GetMinFramerate() const;
    bool          GetUseAllMeans() const;
    unsigned int  GetLODDelay() const;
    unsigned int  GetActiveTS() const;
    unsigned int  GetInactiveTS() const;

    bool          GetWriteLogFile() const;
    bool          GetShowCrashDialog() const;

    const std::wstring GetLogFileName() const;
    uint32_t        GetLogLevel() const;

    bool          GetShowVersionInTitle() const;
    bool          GetAutoSaveGEO() const;
    bool          GetAutoSaveWSP() const;
    bool          GetAutoLockClonedWindow() const;
    bool          GetAbsoluteViewLocks() const;
    bool          GetCheckForUpdatesOnStartUp() const;
    bool          GetCheckForDevBuilds() const;
    bool          GetShowWelcomeScreen() const;
    bool          GetInvertWheel() const;
    bool          GetI3MFeatures() const;
    bool          GetExperimentalFeatures() const;

    unsigned int  GetVolrenType() const;
    FLOATVECTOR3  GetBackgroundColor1() const;
    FLOATVECTOR3  GetBackgroundColor2() const;
    FLOATVECTOR4  GetTextColor() const;
    unsigned int  GetBlendPrecisionMode() const;

    bool          GetUseOnlyPowerOfTwo() const;
    bool          GetDownSampleTo8Bits() const;
    bool          GetDisableBorder() const;
    bool          GetNoRCClipplanes() const;

    QString       GetLogoFilename() const;
    int           GetLogoPos() const;

    unsigned int  GetMaxBrickSize() const;
    unsigned int  GetBuilderBrickSize() const;
    bool GetMedianFilter() const;
    bool GetClampToEdge() const;
    uint32_t GetCompression() const;
    uint32_t GetCompressionLevel() const;
    uint32_t GetLayout() const;
    bool GetUseBasicSettings() const;

    void Data2Form(bool bIsDirectX10Capable,
                   uint64_t iMaxCPU, uint64_t iMaxGPU,
                   bool bIgnoreMax,
                   unsigned int iUserMaxCPUMB, unsigned int iUserMaxGPUMB,
                   const std::wstring& tempDir,
                   bool bQuickopen,
                   unsigned int iMinFramerate,
                   bool bRenderLowResIntermediateResults,
                   unsigned int iLODDelay,
                   unsigned int iActiveTS,
                   unsigned int iInactiveTS,
                   bool bWriteLogFile,
                   bool bShowCrashDialog,
                   const std::wstring& strLogFileName,
                   uint32_t iLogLevel,
                   bool bShowVersionInTitle,
                   bool bAutoSaveGEO,
                   bool bAutoSaveWSP,
                   bool bAutoLockClonedWindow,
                   bool bAbsoluteViewLocks,
                   bool bCheckForUpdatesOnStartUp,
                   bool bCheckForDevBuilds,
                   bool bShowWelcomeScreen,
                   bool bInvWheel,
                   bool bI3MFeatures,
                   unsigned int iVolRenType,
                   unsigned int iBlendPrecision,
                   bool bPowerOfTwo,
                   bool bDownSampleTo8Bits,
                   bool bDisableBorder,
                   const FLOATVECTOR3& vBackColor1,
                   const FLOATVECTOR3& vBackColor2,
                   const FLOATVECTOR4& vTextColor,
                   const QString& strLogo,
                   int iLogoPos,
                   unsigned int iMaxBrickSize,
                   unsigned int iBuilderBrickSize,
                   unsigned int iMaxMaxBrickSize,
                   bool bMedianFilter,
                   bool bClampToEdge,
                   uint32_t iCompression,
                   uint32_t iCompressionLevel,
                   bool expFeatures,
                   uint32_t iLayout);

  protected slots:
    void MemMaxChanged();
    void OverrideMaxToggled(bool bOverride);
    void SelectTextColor();
    void SetTextOpacity(int iOpacity);
    void SelectBackColor1();
    void SelectBackColor2();
    void SetMaxMemCheck();
    void LODDelayChanged();
    void MinFramerateChanged();
    void ActTSChanged();
    void InactTSChanged();
    void WarnAPIMethodChange();
    void SelectLogo();
    void RemoveLogo();
    void PickLogFile();
    void SelectTempDir();
    void MaxBSChanged(int iValue);
    void BuilderBSChanged(int iValue);
    void ToggleExperimentalFeatures();
    void SwitchToBasic();
    void CompressionLevelChanged(int iValue);
    void CompressionChanged();

  private:
    MasterController& m_MasterController;
    QColor            m_cBackColor1;
    QColor            m_cBackColor2;
    QColor            m_cTextColor;
    QString           m_strLogoFilename;
    bool              m_bInit;
    int               m_InitialGPUMemMax;
    bool              m_bWarnAPIChange;
    bool              m_bBasic;

    void setupUi(QDialog *SettingsDlg);
    void SetLogoLabel();
    void MaxToSliders(unsigned int iMaxCPUMB, unsigned int iMaxGPUMB);
};
#endif // SETTINGSDLG_H
