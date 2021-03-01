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

#include "../Tuvok/StdTuvokDefines.h"
#include "../Tuvok/Basics/MathTools.h"
#include "SettingsDlg.h"
#include <QtWidgets/QColorDialog>
#include <QtWidgets/QMessageBox>
#include "../Tuvok/Basics/SysTools.h"
#include "../Tuvok/Basics/SystemInfo.h"
#include <QtWidgets/QFileDialog>
#include <QtCore/QSettings>


using namespace std;

SettingsDlg::SettingsDlg(bool bWarnAPIChange, MasterController& MasterController, QWidget* parent /* = 0 */, Qt::WindowFlags flags /* = 0 */) :
  QDialog(parent, flags),
  m_MasterController(MasterController),
  m_bInit(true),
  m_InitialGPUMemMax(0),
  m_bWarnAPIChange(bWarnAPIChange),
  m_bBasic(false)
{
  setupUi(this);
}

SettingsDlg::~SettingsDlg(void)
{
}

void SettingsDlg::setupUi(QDialog *SettingsDlg) {
  Ui_SettingsDlg::setupUi(SettingsDlg);

  frame_ignoreMax->setVisible(false);

  uint64_t iMaxCPUMemSize   = m_MasterController.SysInfo()->GetCPUMemSize();
  uint64_t iMaxGPUMemSize   = m_MasterController.SysInfo()->GetGPUMemSize();
  unsigned int iProcCount = m_MasterController.SysInfo()->GetNumberOfCPUs();
  unsigned int iBitWidth  = m_MasterController.SysInfo()->GetProgramBitWidth();

  label_Warning32Bit->setVisible(iBitWidth == 32);

  // init stats labels
  QString desc;
  if (m_MasterController.SysInfo()->IsCPUSizeComputed())
    desc = tr("CPU Mem: %1 MB (%2 bytes)").arg(iMaxCPUMemSize/(1024*1024)).arg(iMaxCPUMemSize);
  else
    desc = tr("CPU Mem: unchecked");
  label_CPUMem->setText(desc);

  if (m_MasterController.SysInfo()->IsGPUSizeComputed())
    desc = tr("GPU Mem: %1 MB (%2 bytes)").arg(iMaxGPUMemSize/(1024*1024)).arg(iMaxGPUMemSize);
  else
    desc = tr("GPU Mem: unchecked");

  label_GPUMem->setText(desc);

  if (m_MasterController.SysInfo()->IsNumberOfCPUsComputed())
    desc = tr("Processors %1").arg(iProcCount);
  else
    desc = tr("Processors: unchecked");
  label_NumProc->setText(desc);

  desc = tr("Running in %1 bit mode").arg(iBitWidth);
  label_NumBits->setText(desc);


  unsigned int iMaxCPUMB=static_cast<unsigned int>(iMaxCPUMemSize/(1024*1024));
  unsigned int iMaxGPUMB=static_cast<unsigned int>(iMaxGPUMemSize/(1024*1024));

  MaxToSliders(iMaxCPUMB, iMaxGPUMB);

  // init mem sliders
  horizontalSlider_GPUMem->setMinimum(1024);
  horizontalSlider_CPUMem->setMinimum(2048);

  ToggleExperimentalFeatures();
}

void SettingsDlg::MaxToSliders(unsigned int iMaxCPUMB, unsigned int iMaxGPUMB) {
  horizontalSlider_CPUMem->setMaximum(iMaxCPUMB);
  if (!m_MasterController.SysInfo()->IsCPUSizeComputed()) {
    horizontalSlider_CPUMem->setValue(1024); // choose one gig as default in core size if the max size is not-computed the default
  } else {
    horizontalSlider_CPUMem->setValue(int(iMaxCPUMB*0.8f));
  }

  // on a 32bit system allow only a maximum of 2 gig to be adressed
  if (m_MasterController.SysInfo()->GetProgramBitWidth() == 32)
    horizontalSlider_CPUMem->setMaximum(min(horizontalSlider_CPUMem->maximum(), 2048));

  horizontalSlider_GPUMem->setMaximum(iMaxGPUMB);
  m_InitialGPUMemMax = iMaxGPUMB;
  if (!m_MasterController.SysInfo()->IsGPUSizeComputed()) {
    horizontalSlider_GPUMem->setValue(1024); // choose 1024 meg as default in core size if the max size is not-computed the default
  } else {
    horizontalSlider_GPUMem->setValue(int(iMaxGPUMB*0.8f));
  }
}

bool SettingsDlg::OverrideMaxMem() const {
  return checkBox_OverrideMax->isChecked();
}

unsigned int SettingsDlg::GetMaxGPUMem() const {
  return checkBox_OverrideMax->isChecked() ? spinBox_GPUMax->value() : 0;
}

unsigned int SettingsDlg::GetMaxCPUMem() const {
  return checkBox_OverrideMax->isChecked() ? spinBox_CPUMax->value() : 0;
}

uint64_t SettingsDlg::GetGPUMem() const {
  return uint64_t(horizontalSlider_GPUMem->value())*1024*1024;
}

uint64_t SettingsDlg::GetCPUMem() const {
  return uint64_t(horizontalSlider_CPUMem->value())*1024*1024;
}

std::wstring SettingsDlg::GetTempDir() const {
  return lineEdit_TempDir->text().toStdWString();
}

bool SettingsDlg::GetQuickopen() const {
  return checkBoxQuickload->checkState() == Qt::Checked;
}

unsigned int SettingsDlg::GetMinFramerate() const {
  return (unsigned int)(horizontalSlider_MinFramerate->value());
}

bool SettingsDlg::GetUseAllMeans() const {
  return checkBox_useAllMeans->checkState() == Qt::Checked;
}

unsigned int SettingsDlg::GetLODDelay() const {
  return (unsigned int)(horizontalSlider_LODDelay->value());
}

unsigned int SettingsDlg::GetActiveTS() const {
  return (unsigned int)(horizontalSlider_ActTS->value());
}

unsigned int SettingsDlg::GetInactiveTS() const {
  return (unsigned int)(horizontalSlider_InactTS->value());
}

bool  SettingsDlg::GetShowVersionInTitle() const {
  return checkBox_ShowVersionInTitle->isChecked();
}

bool  SettingsDlg::GetAutoSaveGEO() const {
  return checkBox_SaveGEOOnExit->isChecked();
}

bool  SettingsDlg::GetAutoSaveWSP() const {
  return checkBox_SaveWSPOnExit->isChecked();
}

bool SettingsDlg::GetAutoLockClonedWindow() const {
  return checkBox_AutoLockClonedWindow->isChecked();
}

bool SettingsDlg::GetAbsoluteViewLocks() const {
  return checkBox_AbsoluteViewLocks->isChecked();
}

bool SettingsDlg::GetCheckForUpdatesOnStartUp() const {
  return checkBox_CheckForUpdatesOnStartUp->isChecked();
}

bool SettingsDlg::GetCheckForDevBuilds() const {
  return checkBox_CheckForDevBuilds->isChecked();
}

bool SettingsDlg::GetShowWelcomeScreen() const {
  return checkBox_ShowWelcomeScreen->isChecked();
}

bool SettingsDlg::GetInvertWheel() const {
  return checkBox_InvWheel->isChecked();
}

bool SettingsDlg::GetI3MFeatures() const {
  return checkBox_I3MFeatures->isChecked();
}
bool SettingsDlg::GetExperimentalFeatures() const {
  return checkBox_ExperimentalFeatures->isChecked();
}

FLOATVECTOR3  SettingsDlg::GetBackgroundColor1() const {
  return FLOATVECTOR3(m_cBackColor1.red()/255.0f,
                      m_cBackColor1.green()/255.0f,
                      m_cBackColor1.blue()/255.0f);
}


FLOATVECTOR3  SettingsDlg::GetBackgroundColor2() const {
  return FLOATVECTOR3(m_cBackColor2.red()/255.0f,
                      m_cBackColor2.green()/255.0f,
                      m_cBackColor2.blue()/255.0f);
}

FLOATVECTOR4  SettingsDlg::GetTextColor() const {
  return FLOATVECTOR4(m_cTextColor.red()/255.0f,
                      m_cTextColor.green()/255.0f,
                      m_cTextColor.blue()/255.0f,
                      m_cTextColor.alpha()/255.0f);
}

unsigned int SettingsDlg::GetBlendPrecisionMode() const {
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
  if (checkBox_OverrideMax->isChecked()) return;

  if (horizontalSlider_GPUMem->value() > horizontalSlider_CPUMem->value()) {
    horizontalSlider_GPUMem->setValue(horizontalSlider_CPUMem->value());
  }
  horizontalSlider_GPUMem->setMaximum(min<int>(horizontalSlider_CPUMem->value(),m_InitialGPUMemMax)  );
}

void SettingsDlg::LODDelayChanged() {
  QString text= tr("%1 ms").arg(horizontalSlider_LODDelay->value());
  label_LODDelayDisplay->setText(text);
}

void SettingsDlg::MinFramerateChanged() {
  if (horizontalSlider_MinFramerate->value() > 0) {
    QString text= tr("%1 fps").arg(horizontalSlider_MinFramerate->value());
    label_MinFrameRateDisplay->setText(text);
  } else {
    label_MinFrameRateDisplay->setText(tr("< 1 fps"));
  }
}

void SettingsDlg::ActTSChanged() {
  QString text= tr("%1 ms").arg(horizontalSlider_ActTS->value());
  label_ActTSDisplay->setText(text);
}

void SettingsDlg::InactTSChanged() {
  QString text= tr("%1 ms").arg(horizontalSlider_InactTS->value());
  label_InactTSDisplay->setText(text);
}


void SettingsDlg::SetLogoLabel() {
  if (m_strLogoFilename.isEmpty()) {
    label_LogoFile->setText("No logo selected");
  } else {
    if (SysTools::FileExists(m_strLogoFilename.toStdWString()) ) {
      label_LogoFile->setText(m_strLogoFilename);
    } else {
      label_LogoFile->setText(m_strLogoFilename + " [File not found]");
    }
  }
}

void SettingsDlg::Data2Form(bool bIsDirectX10Capable, uint64_t iMaxCPU, 
                            uint64_t iMaxGPU, bool bIgnoreMax,
                            unsigned int iUserMaxCPUMB, 
                            unsigned int iUserMaxGPUMB, 
                            const std::wstring& tempDir,
                            bool bQuickopen, unsigned int iMinFramerate, 
                            bool bRenderLowResIntermediateResults, 
                            unsigned int iLODDelay,
                            unsigned int iActiveTS, unsigned int iInactiveTS,
                            bool bWriteLogFile, bool bShowCrashDialog,
                            const std::wstring& strLogFileName, 
                            uint32_t iLogLevel,
                            bool bShowVersionInTitle,
                            bool bAutoSaveGEO, bool bAutoSaveWSP, 
                            bool bAutoLockClonedWindow, bool bAbsoluteViewLocks,
                            bool bCheckForUpdatesOnStartUp, 
                            bool bCheckForDevBuilds, bool bShowWelcomeScreen,
                            bool bInvWheel, bool bI3MFeatures,
                            unsigned int iVolRenType, 
                            unsigned int iBlendPrecision, bool bPowerOfTwo, 
                            bool bDownSampleTo8Bits,
                            bool bDisableBorder,
                            const FLOATVECTOR3& vBackColor1,
                            const FLOATVECTOR3& vBackColor2,
                            const FLOATVECTOR4& vTextColor,
                            const QString& strLogo, int iLogoPos,
                            unsigned int iMaxBrickSize,
                            unsigned int iBuilderBrickSize,
                            unsigned int iMaxMaxBrickSize,
                            bool bMedianFilter,
                            bool bClampToEdge,
                            uint32_t iCompression,
                            uint32_t iCompressionLevel,
                            bool expFeatures,
                            uint32_t iLayout) {
  m_bInit = true;

  checkBox_OverrideMax->setChecked(bIgnoreMax);
  if (bIgnoreMax) {
    spinBox_CPUMax->setValue(iUserMaxCPUMB);
    spinBox_GPUMax->setValue(iUserMaxGPUMB);
  } else {
    spinBox_CPUMax->setValue(horizontalSlider_CPUMem->maximum());
    spinBox_GPUMax->setValue(horizontalSlider_GPUMem->maximum());
  }

  horizontalSlider_CPUMem->setValue(iMaxCPU / (1024*1024));
  horizontalSlider_GPUMem->setValue(iMaxGPU / (1024*1024));

  lineEdit_TempDir->setText(QString::fromStdWString(tempDir));
  horizontalSlider_BSMax->setMaximum(iMaxMaxBrickSize);
  horizontalSlider_BSMax->setValue(iMaxBrickSize);
  MaxBSChanged(iMaxBrickSize);

  horizontalSlider_BSBuilder->setMaximum(iMaxMaxBrickSize);
  horizontalSlider_BSBuilder->setValue(iBuilderBrickSize);
  BuilderBSChanged(iBuilderBrickSize);

  radioButton_Mean->setChecked(!bMedianFilter);
  radioButton_Median->setChecked(bMedianFilter);
  radioButton_Border0->setChecked(!bClampToEdge);
  radioButton_BorderClamp->setChecked(bClampToEdge);

  radioButton_noCompression->setChecked(false);
  radioButton_zlibCompression->setChecked(false);
  radioButton_lzmaCompression->setChecked(false);
  radioButton_lz4Compression->setChecked(false);
  radioButton_bzlibCompression->setChecked(false);
  switch (iCompression) {
    default : radioButton_noCompression->setChecked(true); break;
    case 1 : radioButton_zlibCompression->setChecked(true); break;
    case 2 : radioButton_lzmaCompression->setChecked(true); break;
    case 3 : radioButton_lz4Compression->setChecked(true); break;
    case 4 : radioButton_bzlibCompression->setChecked(true); break;
  }
  CompressionLevelChanged(iCompressionLevel);
  CompressionChanged();

  radioButton_scanlineLayout->setChecked(false);
  radioButton_mortonLayout->setChecked(false);
  radioButton_hilbertLayout->setChecked(false);
  radioButton_randomLayout->setChecked(false);
  switch (iLayout) {
    default : radioButton_scanlineLayout->setChecked(true); break;
    case 1 : radioButton_mortonLayout->setChecked(true); break;
    case 2 : radioButton_hilbertLayout->setChecked(true); break;
    case 3 : radioButton_randomLayout->setChecked(true); break;
  }

  checkBoxQuickload->setChecked(bQuickopen);
  horizontalSlider_MinFramerate->setValue(iMinFramerate);
  checkBox_useAllMeans->setChecked(bRenderLowResIntermediateResults);
  horizontalSlider_LODDelay->setValue(iLODDelay);
  horizontalSlider_ActTS->setValue(iActiveTS);
  horizontalSlider_InactTS->setValue(iInactiveTS);

  checkBox_WriteLogfile->setChecked(bWriteLogFile);
  checkBox_ShowCrashDialog->setChecked(bShowCrashDialog);

  lineEdit_filename->setText(QString::fromStdWString(strLogFileName));
  horizontalSlider_loglevel->setValue(iLogLevel);

  checkBox_ShowVersionInTitle->setChecked(bShowVersionInTitle);
  checkBox_SaveGEOOnExit->setChecked(bAutoSaveGEO);
  checkBox_SaveWSPOnExit->setChecked(bAutoSaveWSP);
  checkBox_AutoLockClonedWindow->setChecked(bAutoLockClonedWindow);
  checkBox_AbsoluteViewLocks->setChecked(bAbsoluteViewLocks);
  checkBox_CheckForUpdatesOnStartUp->setChecked(bCheckForUpdatesOnStartUp);
  checkBox_CheckForDevBuilds->setChecked(bCheckForDevBuilds);
  checkBox_ShowWelcomeScreen->setChecked(bShowWelcomeScreen);
  checkBox_InvWheel->setChecked(bInvWheel);
  checkBox_I3MFeatures->setChecked(bI3MFeatures);
  checkBox_ExperimentalFeatures->setChecked(expFeatures);

  m_cBackColor1 = QColor(int(vBackColor1.x*255),
                         int(vBackColor1.y*255),
                         int(vBackColor1.z*255));
  m_cBackColor2 = QColor(int(vBackColor2.x*255),
                         int(vBackColor2.y*255),
                         int(vBackColor2.z*255));
  m_cTextColor  = QColor(int(vTextColor.x*255),
                         int(vTextColor.y*255),
                         int(vTextColor.z*255),
                         int(vTextColor.w*255));

  switch (iBlendPrecision) {
    case 2    : radioButton_Prec32Bit->setChecked(true); break;
    case 1    : radioButton_Prec16Bit->setChecked(true); break;
    default   : radioButton_Prec8Bit->setChecked(true); break;
  }

  radioButton_APIDX->setEnabled(bIsDirectX10Capable);

  // adjust rendermode to GL if no DX support is present but set
  if (!bIsDirectX10Capable) {
    if (iVolRenType == 2) iVolRenType = 0; else 
    if (iVolRenType == 3) iVolRenType = 1; else 
    if (iVolRenType == 5) iVolRenType = 4;
  }


  switch (iVolRenType) {
    case 0   :  radioButton_APIGL->setChecked(true);
                radioButton_SBVR->setChecked(true);
                break;
    case 1    : radioButton_APIGL->setChecked(true);
                radioButton_Raycast->setChecked(true);
                break;
    case 2    : radioButton_APIDX->setChecked(true);
                radioButton_SBVR->setChecked(true);
                break;
    case 3    : radioButton_APIDX->setChecked(true);
                radioButton_Raycast->setChecked(true);
                break;
    case 4    : radioButton_APIGL->setChecked(true);
                radioButton_SBVR2D->setChecked(true);
                break;
    case 5    : radioButton_APIDX->setChecked(true);
                radioButton_SBVR2D->setChecked(true);
                break;
    case 6    : radioButton_APIGL->setChecked(true);
                radioButton_TRaycast->setChecked(true);
                break;
    default   : radioButton_APIDX->setChecked(true);
                radioButton_TRaycast->setChecked(true);
                break;
  }

  switch (iLogoPos) {
    case 0    : radioButton_logoTL->setChecked(true);
                break;
    case 1    : radioButton_logoTR->setChecked(true);
                break;
    case 2    : radioButton_logoBL->setChecked(true);
                break;
    default   : radioButton_logoBR->setChecked(true);
                break;
  }

  m_strLogoFilename = strLogo;
  SetLogoLabel();

  checkBox_PowerOfTwo->setChecked(bPowerOfTwo);
  checkBox_DisableBorder->setChecked(bDisableBorder);
  checkBox_DownSampleTo8Bits->setChecked(bDownSampleTo8Bits);

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
  m_bInit = false;
}

bool SettingsDlg::GetWriteLogFile() const {
  return checkBox_WriteLogfile->isChecked();
}

bool SettingsDlg::GetShowCrashDialog() const {
  return checkBox_ShowCrashDialog->isChecked();
}

const wstring SettingsDlg::GetLogFileName() const {
  return lineEdit_filename->text().toStdWString();
}

uint32_t SettingsDlg::GetLogLevel() const {
  return horizontalSlider_loglevel->value();
}

void SettingsDlg::WarnAPIMethodChange() {
  if (m_bWarnAPIChange && !m_bInit) {
    QMessageBox::warning(this, "Warning", "A change to the render API, the rendermode, or the compatibility settings only affects new render windows.");
    m_bWarnAPIChange = false;
  }
}


bool SettingsDlg::GetMedianFilter() const {
  return radioButton_Median->isChecked();
}

bool SettingsDlg::GetClampToEdge() const {
  return radioButton_BorderClamp->isChecked();
}

uint32_t SettingsDlg::GetCompression() const {
  if (radioButton_noCompression->isChecked()) {
    return 0;
  } else if (radioButton_zlibCompression->isChecked()) {
    return 1;
  } else if (radioButton_lzmaCompression->isChecked()) {
    return 2;
  } else if (radioButton_lz4Compression->isChecked()) {
    return 3;
  } else if (radioButton_bzlibCompression->isChecked()) {
    return 4;
  } else {
    return 1; // default value
  }
}

uint32_t SettingsDlg::GetLayout() const {
  if (radioButton_scanlineLayout->isChecked()) {
    return 0;
  } else if (radioButton_mortonLayout->isChecked()) {
    return 1;
  } else if (radioButton_hilbertLayout->isChecked()) {
    return 2;
  } else if (radioButton_randomLayout->isChecked()) {
    return 3;
  } else {
    return 0; // default value
  }
}

bool SettingsDlg::GetUseBasicSettings() const {
  return m_bBasic;
}

unsigned int SettingsDlg::GetVolrenType() const {
  if (radioButton_APIGL->isChecked()) {
    if (radioButton_SBVR->isChecked()) return 0;
    if (radioButton_SBVR2D->isChecked()) return 4;
    if (radioButton_TRaycast->isChecked()) return 6; else return 1;
  } else {
    if (radioButton_SBVR->isChecked()) return 2;
    if (radioButton_SBVR2D->isChecked()) return 6; else return 3;
  }
}

bool SettingsDlg::GetUseOnlyPowerOfTwo() const {
  return checkBox_PowerOfTwo->isChecked();
}

bool SettingsDlg::GetDownSampleTo8Bits() const {
  return checkBox_DownSampleTo8Bits->isChecked();
}

bool SettingsDlg::GetDisableBorder() const {
  return checkBox_DisableBorder->isChecked();
}

QString SettingsDlg::GetLogoFilename() const {
  return m_strLogoFilename;
}

int SettingsDlg::GetLogoPos() const {
  if (radioButton_logoTL->isChecked()) return 0;
  if (radioButton_logoTR->isChecked()) return 1;
  if (radioButton_logoBL->isChecked()) return 2;
  return 3;
}

void SettingsDlg::MaxBSChanged(int iValue) {
  QString str = tr("%1^3").arg(MathTools::Pow2(uint32_t(iValue)));
  if (iValue < horizontalSlider_BSBuilder->value()) {
    horizontalSlider_BSBuilder->setValue(iValue);
    BuilderBSChanged(iValue);
  }
  label_BSMaxUnit->setText(str);
}

void SettingsDlg::BuilderBSChanged(int iValue) {
  QString str = tr("%1^3").arg(MathTools::Pow2(uint32_t(iValue)));
  if (horizontalSlider_BSMax->value() < iValue) {
    horizontalSlider_BSMax->setValue(iValue);
    MaxBSChanged(iValue);
  }
  label_BSBuilderUnit->setText(str);
}

unsigned int SettingsDlg::GetMaxBrickSize() const {
  return horizontalSlider_BSMax->value();
}

unsigned int SettingsDlg::GetBuilderBrickSize() const {
  return horizontalSlider_BSBuilder->value();
}

void SettingsDlg::CompressionLevelChanged(int iValue) {
  QString str;
  switch (iValue) {
  case  1: str = tr("Fastest"); break;
  case  2: str = tr("Very Fast"); break;
  case  3: str = tr("Fast"); break;
  case  4: str = tr("Quite Fast"); break;
  case  5: str = tr("Normal"); break;
  case  6: str = tr("Quite Strong"); break;
  case  7: str = tr("Strong"); break;
  case  8: str = tr("Very Strong"); break;
  case  9: str = tr("Strongest"); break;
  case 10: str = tr("Ultra"); break;
  }
  str.append(tr(" (%1)").arg(uint32_t(iValue)));
  if (horizontalSlider_CompressionLevel->value() != iValue)
    horizontalSlider_CompressionLevel->setValue(iValue);
  label_CompressionLevelUnit->setText(str);
}

void SettingsDlg::CompressionChanged() {
  frame_CompressionLevel->setEnabled(!radioButton_noCompression->isChecked());
}

uint32_t SettingsDlg::GetCompressionLevel() const {
  return horizontalSlider_CompressionLevel->value();
}

void SettingsDlg::PickLogFile() {
  QSettings settings;
  QString strLastDir = settings.value("Folders/LogFile", ".").toString();

  QFileDialog::Options options;
#ifdef DETECTED_OS_APPLE
  options |= QFileDialog::DontUseNativeDialog;
#endif
  QString selectedFilter;

  QString fileName =
    QFileDialog::getSaveFileName(this, "Select Logfile", strLastDir,
         "All Files (*.*)",&selectedFilter, options);

  if (!fileName.isEmpty()) {
    QDir qdir(QDir::current());
    QString strRelFilename = qdir.relativeFilePath(fileName);

    if (!strRelFilename.contains("..")) fileName = strRelFilename;

    settings.setValue("Folders/LogFile", QFileInfo(fileName).absoluteDir().path());

    lineEdit_filename->setText(fileName);
  }

}

void SettingsDlg::SelectLogo() {
  QSettings settings;
  QString strLastDir = settings.value("Folders/LogoLoad", ".").toString();

  QFileDialog::Options options;
#ifdef DETECTED_OS_APPLE
  options |= QFileDialog::DontUseNativeDialog;
#endif
  QString selectedFilter;

  QString fileName =
    QFileDialog::getOpenFileName(this, "Select Logo", strLastDir,
         "All Files (*.*)",&selectedFilter, options);

  if (!fileName.isEmpty()) {

    QDir qdir(QDir::current());
    QString strRelFilename = qdir.relativeFilePath(fileName);

    if (!strRelFilename.contains("..")) fileName = strRelFilename;

    settings.setValue("Folders/LogoLoad", QFileInfo(fileName).absoluteDir().path());
    m_strLogoFilename = fileName;
  }

  SetLogoLabel();
}

void SettingsDlg::RemoveLogo() {
  m_strLogoFilename = "";
  SetLogoLabel();
}


void SettingsDlg::SelectTempDir() {
  QString directoryName =
    QFileDialog::getExistingDirectory(this, "Select directory for temporary files",lineEdit_TempDir->text());

  if (!directoryName.isEmpty()) {
    if (directoryName[directoryName.size()-1] != '/' &&
        directoryName[directoryName.size()-1] != '\\') directoryName = directoryName+'/';
    lineEdit_TempDir->setText(directoryName);
  }
}


void SettingsDlg::MemMaxChanged() {
  if (checkBox_OverrideMax->isChecked()) {
    horizontalSlider_CPUMem->setMaximum(spinBox_CPUMax->value());
    horizontalSlider_GPUMem->setMaximum(spinBox_GPUMax->value());
  }
}

void SettingsDlg::OverrideMaxToggled(bool bOverride) {
  if (bOverride) {
    horizontalSlider_CPUMem->setMaximum(spinBox_CPUMax->value());
    horizontalSlider_GPUMem->setMaximum(spinBox_GPUMax->value());
  } else {
    uint64_t iMaxCPUMemSize   = m_MasterController.SysInfo()->GetCPUMemSize();
    uint64_t iMaxGPUMemSize   = m_MasterController.SysInfo()->GetGPUMemSize();
    MaxToSliders(iMaxCPUMemSize/(1024*1024), iMaxGPUMemSize/(1024*1024));
  }
}

void SettingsDlg::ToggleExperimentalFeatures() {
#if defined(_WIN32) && defined(USE_DIRECTX)
  if(GetExperimentalFeatures()) {
    groupBox_7->setVisible(true);
  } else {
    groupBox_7->setVisible(false);
    radioButton_APIGL->setChecked(true);
  }
#else
  groupBox_7->setVisible(false);
  radioButton_APIGL->setChecked(true);
#endif

  if(GetExperimentalFeatures()) {
    radioButton_TRaycast->setVisible(true);
    groupBox_Filter->setVisible(true);
  } else {
    radioButton_TRaycast->setVisible(false);
    groupBox_Filter->setVisible(false);
    if (radioButton_TRaycast->isChecked()) 
      radioButton_Raycast->setChecked(true);
  }
}

void SettingsDlg::SwitchToBasic() {
  m_bBasic = true;
  this->reject();
}
