#ifndef IV3D_BASIC_SETTINGS_DLG_H
#define IV3D_BASIC_SETTINGS_DLG_H

#include <QtWidgets/QDialog>
#include "Basics/Vectors.h"

class QLabel;
class QSlider;

namespace tuvok { class MasterController; }

enum PerformanceLevel {
  MAX_RESPONSIVENESS=0,
  MIDDLE_1,
  MAX_PERFORMANCE,
};

class BasicSettingsDlg : public QDialog
{
  Q_OBJECT
  public:
    BasicSettingsDlg(tuvok::MasterController& masterController,
                     enum PerformanceLevel defaultLevel = MAX_RESPONSIVENESS,
                     QWidget* parent = 0,
                     Qt::WindowFlags flags = 0);
    virtual ~BasicSettingsDlg();

    enum PerformanceLevel GetPerformanceLevel() const;
    bool GetUseAdvancedSettings() const;

    void Data2Form(bool bIsDirectX10Capable,
                   uint64_t iMaxCPU, uint64_t iMaxGPU,
                   bool bIgnoreMax,
                   unsigned int iUserMaxCPUMB, unsigned int iUserMaxGPUMB,
                   const std::string& tempDir,
                   bool bQuickopen,
                   unsigned int iMinFramerate,
                   bool bRenderLowResIntermediateResults,
                   unsigned int iLODDelay,
                   unsigned int iActiveTS,
                   unsigned int iInactiveTS,
                   bool bWriteLogFile,
                   bool bShowCrashDialog,
                   const std::string& strLogFileName,
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
                   bool expFeatures,
                   bool advFeatures);

  protected slots:
    void PerfLevelChanged(int level);
    void AdvancedFeatures();

  private:
    tuvok::MasterController& ctlr;
    QLabel* lDesc; // so we can change it in handlers.
    QSlider* slPerf;
    bool useAdvanced;

    void setupUI(QDialog *BasicSettingsDlg, enum PerformanceLevel);
};
#endif
/*
   For more information, please see: http://software.sci.utah.edu

   The MIT License

   Copyright (c) 2012 Scientific Computing and Imaging Institute,
                      Interactive Visualization and Data Analysis Group


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
