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


//!    File   : ImageVis3D.cpp
//!    Author : Jens Krueger
//!             SCI Institute
//!             University of Utah
//!    Date   : July 2008
//
//!    Copyright (C) 2008 SCI Institute

#include "StdDefines.h"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <string>
#include <QtCore/QFileInfo>
#include <QtCore/QSettings>
#include <QtCore/QTimer>
#include <QtGui/QCloseEvent>
#include <QtWidgets/QColorDialog>
#include <QtWidgets/QMdiSubWindow>
#include <QtWidgets/QMessageBox>
#include <QtNetwork/QNetworkReply>
#include "../Tuvok/Basics/SystemInfo.h"
#include "../Tuvok/Basics/SysTools.h"
#include "../Tuvok/Basics/MathTools.h"
#include "../IO/DialogConverter.h"
#include "../Tuvok/LuaScripting/TuvokSpecific/LuaTuvokTypes.h"
#include "BrowseData.h"
#include "CrashDetDlg.h"
#include "FTPDialog.h"
#include "ImageVis3D.h"
#include "PleaseWait.h"

using namespace std;

MainWindow::MainWindow(MasterController& masterController,
           bool bScriptMode, /* = false */
           QWidget* parent /* = 0 */,
           Qt::WindowFlags flags /* = 0 */) :
  QMainWindow(parent, flags),
  m_pRedrawTimer(nullptr),
  m_MasterController(masterController),
  m_strCurrentWorkspaceFilename(""),
  m_strTempDir(L"."),  // changed in the constructor
  m_bShowVersionInTitle(true),
  m_bQuickopen(true),
  m_iMinFramerate(10),
  m_iLODDelay(1000),
  m_bRenderLowResIntermediateResults(false),
  m_iActiveTS(500),
  m_iInactiveTS(100),
  m_bWriteLogFile(false),
  m_strLogFileName("debugLog.txt"),
  m_iLogLevel(2),
  m_pWelcomeDialog(new WelcomeDialog(this, Qt::Tool)),
  m_pMetadataDialog(new MetadataDlg(this, Qt::Tool)),
  m_pDebugScriptWindow(new DebugScriptWindow(masterController, this)),
  m_iBlendPrecisionMode(0),
  m_bPowerOfTwo(true),
  m_bDownSampleTo8Bits(false),
  m_bDisableBorder(false),
  m_bI3MFeatures(false),
  m_bAdvancedSettings(false),
  m_bAutoSaveGEO(true),
  m_bAutoSaveWSP(true),
  m_eVolumeRendererType(MasterController::OPENGL_SBVR),
  m_bUpdatingLockView(false),
  m_strLogoFilename(""),
  m_iLogoPos(3),
  m_bAutoLockClonedWindow(false),
  m_bAbsoluteViewLocks(true),
  m_bCheckForUpdatesOnStartUp(false),
  m_bCheckForDevBuilds(false),
  m_bShowWelcomeScreen(true),
  m_bInvWheel(true),
  m_bStayOpenAfterScriptEnd(false),
  m_pTextout(nullptr),
  m_pActiveRenderWin(nullptr),
  m_pLastLoadedRenderWin(nullptr),
  m_pUpdateFile(nullptr),
  m_pHttpReply(nullptr),
  m_bStartupCheck(false),
  m_bScriptMode(bScriptMode),
  m_pFTPDialog(nullptr),
  m_strFTPTempFile(L""),
  m_bFTPDeleteSource(true),
  m_bFTPFinished(true),
  m_bClipDisplay(true),
  m_bClipLocked(false),
  m_bIgnoreLoadDatasetFailure(false),
  m_MemReg(m_MasterController.LuaScript())
{
  RegisterLuaClasses();

  QCoreApplication::setOrganizationName("Scientific Computing and Imaging Institute, University of Utah");
  QCoreApplication::setOrganizationDomain(SCI_ORGANIZATION_DOMAIN);
  QCoreApplication::setApplicationName("ImageVis3D");
  QString qstrVersion = tr("%1").arg(IV3D_VERSION);
  QCoreApplication::setApplicationVersion(qstrVersion);

  QString path = QApplication::applicationDirPath();
  masterController.SysInfo()->SetProgramPath(path.toStdWString());

  setupUi(this);

  SysTools::GetTempDirectory(m_strTempDir);

  ApplySettings();
  SetupWorkspaceMenu();

  if (!LoadDefaultGeometry()) SaveDefaultGeometry();

  if (!LoadDefaultWorkspace()) {
    InitAllWorkspaces();
    SaveDefaultWorkspace();
  }

  this->show();
  SetAndCheckRunningFlag();

  shared_ptr<LuaScripting> ss(m_MasterController.LuaScript()); 
  ss->cexec("tuvok.io.registerFinalConverter",
      dynamic_pointer_cast<AbstrConverter>(shared_ptr<DialogConverter>(
          new DialogConverter(this))));

  UpdateMRUActions();
  UpdateWSMRUActions();
  UpdateMenus();

  CheckSettings();
  ClearProgressViewAndInfo();
  UpdateColorWidget();

  connect(m_pWelcomeDialog, SIGNAL(CheckUpdatesClicked()),   this, SLOT(CheckForUpdates()));
  connect(m_pWelcomeDialog, SIGNAL(OnlineVideoTutClicked()), this, SLOT(OnlineVideoTut()));
  connect(m_pWelcomeDialog, SIGNAL(GetExampleDataClicked()), this, SLOT(GetExampleData()));
  connect(m_pWelcomeDialog, SIGNAL(OnlineHelpClicked()),     this, SLOT(OnlineHelp()));
  connect(m_pWelcomeDialog, SIGNAL(OpenManualClicked()),     this, SLOT(OpenManual()));
  connect(m_pWelcomeDialog, SIGNAL(OpenFromFileClicked()),   this, SLOT(LoadDataset()));
  connect(m_pWelcomeDialog, SIGNAL(OpenFromFileClicked(std::wstring)),   this, SLOT(LoadDataset(const std::wstring &)));
  connect(m_pWelcomeDialog, SIGNAL(OpenFromDirClicked()),    this, SLOT(LoadDirectory()));
  connect(m_pWelcomeDialog, SIGNAL(accepted()),              this, SLOT(CloseWelcome()));

  if (!m_bScriptMode && m_bCheckForUpdatesOnStartUp) QuietCheckForUpdates();
  if (!m_bScriptMode && m_bShowWelcomeScreen) ShowWelcomeScreen();

  checkBox_ClipShow->setEnabled(false);
  checkBox_ClipLockObject->setEnabled(false);
  toolButton_CropData->setEnabled(false);
  DisableStereoWidgets();

  m_pRedrawTimer = new QTimer(this);
  m_pRedrawTimer->setInterval(IV3D_TIMER_INTERVAL);

  setAcceptDrops(true);
}

namespace {
  template <typename T> void Delete(T *t) { delete t; }
}

MainWindow::~MainWindow()
{
  if (m_pDebugOut == m_MasterController.DebugOut()) {
    m_MasterController.RemoveDebugOut(m_pDebugOut);
  }

  // cleanup updatefile, this code path is taken for instance when the
  // windows firewall blocked an http request
  if (m_pUpdateFile && m_pUpdateFile->isOpen()) {
    m_pUpdateFile->close();
    m_pUpdateFile->remove();
    delete m_pUpdateFile;
    m_pUpdateFile = nullptr;
  }

  if (m_pHttpReply) {
    m_pHttpReply->deleteLater();
    m_pHttpReply = nullptr;
  }

  m_pRedrawTimer->stop();
  delete m_pRedrawTimer;

  RemoveRunningFlag();

  delete m_pMetadataDialog;
  delete m_pWelcomeDialog;
  delete m_pFTPDialog;

  delete m_1DTransferFunction;
  delete m_2DTransferFunction;

  disconnect(m_pQLightPreview, SIGNAL(lightMoved()), 
             this, SLOT(LightMoved()));

  delete m_pQLightPreview;
  std::for_each(m_recentFileActs, m_recentFileActs+ms_iMaxRecentFiles,
                Delete<QAction>);
  std::for_each(m_recentWSFileActs, m_recentWSFileActs+ms_iMaxRecentFiles,
                Delete<QAction>);
}

/// initializes the timer.
void MainWindow::StartTimer() {
  connect(m_pRedrawTimer, SIGNAL(timeout()), this, SLOT(CheckForRedraw()));
  m_pRedrawTimer->start(IV3D_TIMER_INTERVAL);
}

void MainWindow::SetAndCheckRunningFlag() {
  QSettings settings;
  uint32_t iInstanceCounter = settings.value("InstanceCounter", 0).toUInt();
  uint32_t iSaneCounter = settings.value("SaneCounter", 0).toUInt();

  settings.beginGroup("Performance");
  bool bWriteLogFile = settings.value("WriteLogFile", m_bWriteLogFile).toBool();
  bool bShowCrashDialog = settings.value("ShowCrashDialog", true).toBool();
  QString strLogFileName = settings.value("LogFileName", m_strLogFileName).toString();
  settings.endGroup();

 if (iInstanceCounter) {
    if (bShowCrashDialog) {
      settings.setValue("SaneCounter", 0);
      CrashDetDlg* d = NULL;
      if (bWriteLogFile && SysTools::FileExists(strLogFileName.toStdWString())) {
        d = new CrashDetDlg("Crash recovery", "Either ImageVis3D crashed or it is currently running in a second process. If it crashed do you want to submit the logfile?", false, this);
        if (d->exec() == QDialog::Accepted) {
          ReportABug(strLogFileName.toStdWString());
          SysTools::RemoveFile(strLogFileName.toStdWString());
        }
      } else {
        if (!bWriteLogFile) {
          d = new CrashDetDlg("Crash recovery", "Either ImageVis3D crashed or it is currently running in a second process. If it crashed do you want to enable debugging?", false, this);
          if (d->exec() == QDialog::Accepted) {
            settings.setValue("Performance/WriteLogFile", true);
            settings.setValue("Performance/LogLevel", 2);
          } else {
            // if debugging was not enabled assume that has been opened multiple times
            settings.setValue("SaneCounter", (unsigned int)iSaneCounter);
          }
        }
      }
      iInstanceCounter = 0;
      if (d) {
        settings.setValue("Performance/ShowCrashDialog", !d->GetDontShowAgain());
        delete d;
      }
    }
  } else {
    if (bShowCrashDialog && bWriteLogFile && iSaneCounter > 10) {
      CrashDetDlg* d = new CrashDetDlg("Crash recovery", "ImageVis3D did not crash the last 10 times it was used. Do you want to disable debug logging now?", false, this);
      if (d->exec() == QDialog::Accepted) {
        settings.setValue("Performance/WriteLogFile", false);
      }
      settings.setValue("Performance/ShowCrashDialog", !d->GetDontShowAgain());
      delete d;
    }
  }

  settings.setValue("InstanceCounter", (unsigned int)iInstanceCounter+1);
}

void MainWindow::RemoveRunningFlag() {
  QSettings settings;
//  uint32_t iInstanceCounter = settings.value("InstanceCounter", 1).toUInt();
  settings.setValue("InstanceCounter", 0);

  uint32_t iSaneCounter = settings.value("SaneCounter", 0).toUInt();
  settings.setValue("SaneCounter", (unsigned int)iSaneCounter+1);
}

// ******************************************
// Filter Function Dock
// ******************************************


// ******************************************
// Render Mode
// ******************************************

void MainWindow::Use1DTrans() {
  if (!m_pActiveRenderWin) return;

  label_2DIndicator->setVisible(false);
  label_2DIndicator_SimpleUI->setVisible(false);
  label_IsoDIndicator->setVisible(false);

  checkBox_Use2DTrans->setChecked(false);
  checkBox_Use2DTrans->setVisible(true);
  checkBox_Use2DTrans_SimpleUI->setChecked(false);
  checkBox_Use2DTrans_SimpleUI->setVisible(true);
  checkBox_UseIso->setChecked(false);
  checkBox_UseIso->setVisible(true);
  checkBox_Use1DTrans->setChecked(true);
  checkBox_Use1DTrans->setVisible(false);
  radioButton_1DTrans->setChecked(true);

  label_1DIndicator->setVisible(true);

  checkBox_Lighting->setEnabled(true);

  toolButton_ExpIsoToMesh->setEnabled(false);
  m_1DTransferFunction->setEnabled(true);
  m_2DTransferFunction->setEnabled(false);

  if (m_pActiveRenderWin) m_pActiveRenderWin->SetRendermode(AbstrRenderer::RM_1DTRANS);
}


void MainWindow::Use2DTrans() {
  if (!m_pActiveRenderWin) return;

  frame_Simple2DTransControls->setEnabled(true);

  label_1DIndicator->setVisible(false);
  label_IsoDIndicator->setVisible(false);

  checkBox_Use1DTrans->setChecked(false);
  checkBox_Use1DTrans->setVisible(true);
  checkBox_UseIso->setChecked(false);
  checkBox_UseIso->setVisible(true);
  checkBox_Use2DTrans_SimpleUI->setChecked(true);
  checkBox_Use2DTrans_SimpleUI->setVisible(false);
  checkBox_Use2DTrans->setChecked(true);
  checkBox_Use2DTrans->setVisible(false);
  radioButton_2DTrans->setChecked(true);

  label_2DIndicator->setVisible(true);
  label_2DIndicator_SimpleUI->setVisible(true);
  label_2DIndicator->setText("2D Transfer Function Enabled");

  checkBox_Lighting->setEnabled(true);

  toolButton_ExpIsoToMesh->setEnabled(false);
  m_1DTransferFunction->setEnabled(false);
  m_2DTransferFunction->setEnabled(true);

  frame_top_simpleUI->setEnabled(true);
  frame_center_simpleUI->setEnabled(true);
  frame_bottomL_simpleUI->setEnabled(true);
  frame_2DTransEdit->setEnabled(true);

  int iCurrent = m_2DTransferFunction->GetActiveSwatchIndex();
  if (iCurrent < 0 && m_2DTransferFunction->GetSwatchCount() > 0) {
    m_2DTransferFunction->Transfer2DSetActiveSwatch(0);
    Transfer2DSwatchesChanged();
  }

  if (m_pActiveRenderWin) m_pActiveRenderWin->SetRendermode(AbstrRenderer::RM_2DTRANS);
}


void MainWindow::UseIso() {
  if (!m_pActiveRenderWin) return;

  label_1DIndicator->setVisible(false);
  label_2DIndicator->setVisible(false);
  label_2DIndicator->setText("");
  label_2DIndicator_SimpleUI->setVisible(false);

  checkBox_Use2DTrans->setChecked(false);
  checkBox_Use2DTrans->setVisible(true);
  checkBox_Use2DTrans_SimpleUI->setChecked(false);
  checkBox_Use2DTrans_SimpleUI->setVisible(true);
  checkBox_Use1DTrans->setChecked(false);
  checkBox_Use1DTrans->setVisible(true);
  checkBox_UseIso->setChecked(true);
  checkBox_UseIso->setVisible(false);
  radioButton_Iso->setChecked(true);

  label_IsoDIndicator->setVisible(true);

  checkBox_Lighting->setEnabled(false);

  toolButton_ExpIsoToMesh->setEnabled(true);
  m_1DTransferFunction->setEnabled(false);
  m_2DTransferFunction->setEnabled(false);

  if (m_pActiveRenderWin) m_pActiveRenderWin->SetRendermode(AbstrRenderer::RM_ISOSURFACE);
}


void MainWindow::DisableAllTrans() {
  label_1DIndicator->setVisible(false);
  label_2DIndicator->setVisible(false);
  label_2DIndicator->setText("");
  label_2DIndicator_SimpleUI->setVisible(false);
  label_IsoDIndicator->setVisible(false);

  checkBox_Use2DTrans_SimpleUI->setChecked(false);
  checkBox_Use2DTrans_SimpleUI->setVisible(false);
  checkBox_Use2DTrans->setChecked(false);
  checkBox_Use2DTrans->setVisible(false);
  checkBox_UseIso->setChecked(false);
  checkBox_UseIso->setVisible(false);
  checkBox_Use1DTrans->setChecked(false);
  checkBox_Use1DTrans->setVisible(false);

  m_1DTransferFunction->setEnabled(false);
  m_2DTransferFunction->setEnabled(false);

  frame_top_simpleUI->setEnabled(false);
  frame_center_simpleUI->setEnabled(false);
  frame_bottomL_simpleUI->setEnabled(false);
  frame_2DTransEdit->setEnabled(false);

  listWidget_Swatches->clear();
}

void MainWindow::SetLighting(bool bLighting) {
  if (m_pActiveRenderWin != NULL) m_pActiveRenderWin->SetUseLighting(bLighting);
}

void MainWindow::ToggleLighting() {
  if (m_pActiveRenderWin != NULL) {
    m_pActiveRenderWin->SetUseLighting(!m_pActiveRenderWin->GetUseLighting());
  }
}

void MainWindow::SetSampleRate(int iValue) {
  if (m_pActiveRenderWin != NULL) m_pActiveRenderWin->SetSampleRateModifier(iValue/100.0f);
  UpdateSampleRateLabel(iValue);
}

void MainWindow::UpdateSampleRateLabel(int iValue) {
  QString desc;
  desc = tr("Sampling Rate (%1%):").arg(iValue);
  label_SamplingRate->setText(desc);
}

void MainWindow::SetSampleRateSlider(int iValue) {
  horizontalSlider_Sampling->setValue(iValue);
  UpdateSampleRateLabel(iValue);
}

void MainWindow::SetFoV(int iValue) {
  if (m_pActiveRenderWin != NULL) m_pActiveRenderWin->SetFoV(iValue);
  UpdateFoVLabel(iValue);
}

void MainWindow::SetFoVSlider(int iValue) {
  horizontalSlider_FoV->setValue(iValue);
  UpdateFoVLabel(iValue);
}

void MainWindow::UpdateFoVLabel(int iValue) {
  QString desc;
  desc = tr("Field of View (%1):").arg(iValue);
  label_FoV->setText(desc);
}

// Script command to update isovalue.
void MainWindow::SetIsoValue(float fValue) {
  if (m_pActiveRenderWin != NULL) {
    int iMaxSize = int(m_pActiveRenderWin->GetDynamicRange().second);
    fValue = MathTools::Clamp(fValue, 0.0f, 1.0f);
    m_pActiveRenderWin->SetIsoValue(fValue * iMaxSize);
    UpdateIsoValLabel(int(fValue * iMaxSize), iMaxSize);
  }
}

// UI command to update isovalue.
void MainWindow::SetIsoValue(int iValue) {
  if (m_pActiveRenderWin != NULL) {
    m_pActiveRenderWin->SetIsoValue(float(iValue));
    int iMaxSize = int(m_pActiveRenderWin->GetDynamicRange().second);
    UpdateIsoValLabel(iValue, iMaxSize);
  }
}

// Script command to update focus isovalue
void MainWindow::SetClearViewIsoValue(float fValue) {
  if (m_pActiveRenderWin != NULL) {
    int iMaxSize = int(m_pActiveRenderWin->GetDynamicRange().second);
    fValue = MathTools::Clamp(fValue, 0.0f, 1.0f);
    m_pActiveRenderWin->SetCVIsoValue(fValue * iMaxSize);
    UpdateIsoValLabel(int(fValue*iMaxSize), iMaxSize);
  }
}

void MainWindow::SetIsoValueSlider(int iValue, int iMaxValue) {
  horizontalSlider_Isovalue->setMaximum(iMaxValue);
  horizontalSlider_Isovalue->setValue(iValue);
}

void MainWindow::UpdateIsoValLabel(int iValue, int iMaxValue) {
  QString desc;
  desc = tr("%1/%2").arg(iValue).arg(iMaxValue);
  label_IsoValue->setText(desc);
}


void MainWindow::SetFocusIsoValue(int iValue) {
  int iMaxSize = int(m_pActiveRenderWin->GetDynamicRange().second);
  if (m_pActiveRenderWin != NULL) {
    m_pActiveRenderWin->SetCVIsoValue(float(iValue));
  }
  UpdateFocusIsoValLabel(iValue, iMaxSize);
}

void MainWindow::SetFocusIsoValueSlider(int iValue, int iMaxValue) {
  horizontalSlider_CVFocusIsoValue->setMaximum(iMaxValue);
  horizontalSlider_CVFocusIsoValue->setValue(iValue);
}

void MainWindow::SetFocusSizeValueSlider(int iValue){
  horizontalSlider_CVFocusSize->setValue(iValue);
}

void MainWindow::SetContextScaleValueSlider(int iValue){
  horizontalSlider_CVContextScale->setValue(iValue);
}

void MainWindow::SetBorderSizeValueSlider(int iValue) {
  horizontalSlider_CVBorder->setValue(iValue);
}

void MainWindow::SetFocusSize(int iValue) {
  if (m_pActiveRenderWin != NULL) m_pActiveRenderWin->SetCVSize(float(99-iValue)/9.9f);
}

void MainWindow::SetContextScale(int iValue) {
  if (m_pActiveRenderWin != NULL) m_pActiveRenderWin->SetCVContextScale(float(iValue)/10.0f);
}

void MainWindow::SetBorderSize(int iValue) {
  if (m_pActiveRenderWin != NULL) m_pActiveRenderWin->SetCVBorderScale(float(99-iValue));
}

void MainWindow::UpdateFocusIsoValLabel(int iValue, int iMaxValue) {
  QString desc;
  desc = tr("%1/%2").arg(iValue).arg(iMaxValue);
  label_CVFocusIsoValue->setText(desc);
}

void MainWindow::SetToggleGlobalBBoxLabel(bool bRenderBBox)
{
  this->checkBox_GBBox->setChecked(bRenderBBox);
}

void MainWindow::SetToggleLocalBBoxLabel(bool bRenderBBox)
{
  this->checkBox_LBBox->setChecked(bRenderBBox);
}

void MainWindow::ToggleGlobalBBox(bool bRenderBBox)
{
  if (m_pActiveRenderWin != NULL) m_pActiveRenderWin->SetGlobalBBox(bRenderBBox);
}

void MainWindow::SetToggleClipEnabledLabel(bool b) {
  this->checkBox_ClipPlane->setChecked(b);
}
void MainWindow::SetToggleClipShownLabel(bool b) {
  this->checkBox_ClipShow->setChecked(b);
}
void MainWindow::SetToggleClipLockedLabel(bool b) {
  this->checkBox_ClipLockObject->setChecked(b);
}

void MainWindow::ToggleLocalBBox(bool bRenderBBox)
{
  if (m_pActiveRenderWin != NULL) m_pActiveRenderWin->SetLocalBBox(bRenderBBox);
}

void MainWindow::ToggleClipPlane(bool bClip)
{
  RenderWindow *rw = m_pActiveRenderWin;
  if(rw == NULL) { return; }

  rw->SetClipPlaneEnabled(bClip);

  LuaCallbackToggleClipPlane(bClip);
}

void MainWindow::LuaCallbackToggleClipPlane(bool bClip)
{
  // Check the 'enable' box, but make sure we don't send any sort of event.
  checkBox_ClipPlane->blockSignals(true);
  checkBox_ClipPlane->setChecked(bClip);
  checkBox_ClipPlane->blockSignals(false);

  checkBox_ClipShow->setEnabled(bClip);
  checkBox_ClipLockObject->setEnabled(bClip);
  toolButton_CropData->setEnabled(bClip);
}

void MainWindow::ClipToggleShow(bool bShow)
{
  RenderWindow *rw = m_pActiveRenderWin;
  if(rw == NULL) { return; }

  rw->SetClipPlaneDisplayed(bShow);
  m_bClipDisplay = bShow;
}

void MainWindow::ClipToggleLock(bool bLock)
{
  RenderWindow *rw = m_pActiveRenderWin;
  if(rw == NULL) { return; }

  rw->SetClipPlaneRelativeLock(bLock);
  m_bClipLocked = bLock;
}

void MainWindow::SetTimestep(int t)
{
  RenderWindow *rw = m_pActiveRenderWin;
  if(rw == NULL) { return; }

  rw->SetTimestep(static_cast<size_t>(t));
  LuaClassInstance ds = m_pActiveRenderWin->GetRendererDataset();
  int iMaxVal = static_cast<int>(
      m_MasterController.LuaScript()->cexecRet<uint64_t>(ds.fqName() + 
                                                     ".getNumberOfTimesteps"));
  UpdateTimestepLabel(int(t), iMaxVal);
}

void MainWindow::SetTimestepSlider(int iValue, int iMaxValue)
{
  hSlider_Timestep->setMaximum(iMaxValue);
  hSlider_Timestep->setValue(iValue);
  hSlider_Timestep->setEnabled(iMaxValue != 1);
}

void MainWindow::UpdateTimestepLabel(int iValue, int iMaxValue) {
  if(iMaxValue == 1) {
    QString desc;
    desc = tr("One");
    label_Timestep->setText(desc);
  } else {
    QString desc;
    desc = tr("%1/%2").arg(iValue+1).arg(iMaxValue);
    label_Timestep->setText(desc);
  }
}
void MainWindow::ResetTimestepUI() {
  SetTimestepSlider(0,1);
  UpdateTimestepLabel(0,1);
}


void MainWindow::closeEvent(QCloseEvent *event)
{
  if (m_bAutoSaveGEO) SaveDefaultGeometry();
  if (m_bAutoSaveWSP) SaveDefaultWorkspace();
  m_MasterController.RemoveDebugOut(m_pDebugOut);
  m_pDebugOut = NULL;
  event->accept();
}

void MainWindow::ChooseIsoColor()
{
  if (m_pActiveRenderWin)  {
    FLOATVECTOR3 vIsoColor = m_pActiveRenderWin->GetIsosurfaceColor();
    QColor color = QColorDialog::getColor(qRgba(int(vIsoColor.x*255),
                                                int(vIsoColor.y*255),
                                                int(vIsoColor.z*255),
                                                255),
                                                this);
    if (color.isValid()) {

      vIsoColor = FLOATVECTOR3(color.red() / 255.0f, color.green() / 255.0f, color.blue() / 255.0f);
      m_pActiveRenderWin->SetIsosurfaceColor(vIsoColor);
    }
  }
}

void MainWindow::ChooseFocusColor()
{
  if (m_pActiveRenderWin)  {
    FLOATVECTOR3 vIsoColor = m_pActiveRenderWin->GetCVColor();
    QColor color = QColorDialog::getColor(qRgba(int(vIsoColor.x*255),
                                                int(vIsoColor.y*255),
                                                int(vIsoColor.z*255),
                                                255),
                                                this);
    if (color.isValid()) {

      vIsoColor = FLOATVECTOR3(color.red() / 255.0f, color.green() / 255.0f, color.blue() / 255.0f);
      m_pActiveRenderWin->SetCVColor(vIsoColor);
    }
  }
}

void MainWindow::ToggleClearView() {
  if (m_pActiveRenderWin) {
    m_pActiveRenderWin->SetCV(checkBox_ClearView->isChecked());
    frame_ClearView->setEnabled(checkBox_ClearView->isChecked());
  }
}

// ******************************************
// Dataset interaction
// ******************************************

void MainWindow::RotateCurrentViewX(double angle) {
  if (m_pActiveRenderWin) {
    FLOATMATRIX4 matRot;
    matRot.RotationX(3.141592653589793238462643383*angle/180.0);
    m_pActiveRenderWin->Rotate(matRot);
  }
}

void MainWindow::RotateCurrentViewY(double angle) {
  if (m_pActiveRenderWin) {
    FLOATMATRIX4 matRot;
    matRot.RotationY(3.141592653589793238462643383*angle/180.0);
    m_pActiveRenderWin->Rotate(matRot);
  }
}

void MainWindow::RotateCurrentViewZ(double angle) {
  if (m_pActiveRenderWin) {
    FLOATMATRIX4 matRot;
    matRot.RotationZ(3.141592653589793238462643383*angle/180.0);
    m_pActiveRenderWin->Rotate(matRot);
  }
}

void MainWindow::TranslateCurrentView(double x, double y, double z) {
  if (m_pActiveRenderWin) {
    FLOATMATRIX4 matTrans;
    matTrans.Translation(x,y,z);
    m_pActiveRenderWin->Translate(matTrans);
  }
}


void MainWindow::ToggleFullscreen() {
  if (m_pActiveRenderWin)
    m_pActiveRenderWin->ToggleFullscreen();
}


QMdiSubWindow* MainWindow::ActiveSubWindow() {
  for (int i = 0;i<mdiArea->subWindowList().size();i++) {
    QWidget* w = mdiArea->subWindowList().at(i)->widget();
    RenderWindow* subwindow = WidgetToRenderWin(w);
    if (subwindow == m_pActiveRenderWin) return mdiArea->subWindowList().at(i);
  }
  return NULL;
}

RenderWindow* MainWindow::ActiveRenderWin() {
  if (mdiArea->activeSubWindow())
    return WidgetToRenderWin(mdiArea->activeSubWindow()->widget());
  else
    return NULL;
}


RenderWindow* MainWindow::WidgetToRenderWin(QWidget* w) {
  // Given Widget might actually be NULL, in the case that window (GL)
  // initialization failed, but Qt got asked to process an interaction event
  // (e.g. a mouse move) before the `close' event.  MainWindow::CheckForRedraw
  // has the same issue; see the comment there for more info.
  if (w == NULL) { return NULL; }

  if (w->objectName() == "RenderWindowGL") {
    RenderWindowGL* r = static_cast<RenderWindowGL*>(w);
    return static_cast<RenderWindow*>(r);
  }
#if defined(_WIN32) && defined(USE_DIRECTX)
  if (w->objectName() == "RenderWindowDX") {
      RenderWindowDX* r = static_cast<RenderWindowDX*>(w);
      return static_cast<RenderWindow*>(r);
  }
#endif
  return NULL;
}

void MainWindow::PickLightColor() {
  if (!m_pActiveRenderWin) return;

  FLOATVECTOR4 cAmbient  = m_pQLightPreview->GetAmbient();
  FLOATVECTOR4 cDiffuse  = m_pQLightPreview->GetDiffuse();
  FLOATVECTOR4 cSpecular = m_pQLightPreview->GetSpecular();
  FLOATVECTOR3 vLightDir = m_pQLightPreview->GetLightDir();

  QPushButton* source = qobject_cast<QPushButton*>(sender());

  FLOATVECTOR4 prevColorVec;
  if (source == pushButton_ambientColor)
    prevColorVec = cAmbient;
  else if (source == pushButton_diffuseColor)
    prevColorVec = cDiffuse;
  else if (source == pushButton_specularColor)
    prevColorVec = cSpecular;
  else
    return;

  // these should probably be a size_t or even unsigned char, but Qt's APIs
  // take an int.
  const int rgb[3] = {
    static_cast<int>(prevColorVec[0] * 255),
    static_cast<int>(prevColorVec[1] * 255),
    static_cast<int>(prevColorVec[2] * 255)
  };
  QColor prevColor(rgb[0], rgb[1], rgb[2]);
  QColor color = QColorDialog::getColor(prevColor, this);

  if (color.isValid()) {
    prevColorVec[0] = color.red()/255.0f;
    prevColorVec[1] = color.green()/255.0f;
    prevColorVec[2] = color.blue()/255.0f;

    if (source == pushButton_ambientColor)
      cAmbient = prevColorVec;
    else if (source == pushButton_diffuseColor)
      cDiffuse = prevColorVec;
    else if (source == pushButton_specularColor)
      cSpecular = prevColorVec;
    else
      return;

    m_pQLightPreview->SetData(cAmbient,cDiffuse,cSpecular,vLightDir);
    m_pActiveRenderWin->SetLightColors(cAmbient, cDiffuse,
                                                 cSpecular, vLightDir);
  }
}


void MainWindow::LightMoved() {
  if (!m_pActiveRenderWin) return;

  FLOATVECTOR4 cAmbient  = m_pQLightPreview->GetAmbient();
  FLOATVECTOR4 cDiffuse  = m_pQLightPreview->GetDiffuse();
  FLOATVECTOR4 cSpecular = m_pQLightPreview->GetSpecular();
  FLOATVECTOR3 vLightDir = m_pQLightPreview->GetLightDir();

  m_pActiveRenderWin->SetLightColors(cAmbient, cDiffuse,
                                     cSpecular, vLightDir);
}

void MainWindow::ChangeLightColors() {
  if (!m_pActiveRenderWin) return;

  FLOATVECTOR4 cAmbient  = m_pQLightPreview->GetAmbient();
  FLOATVECTOR4 cDiffuse  = m_pQLightPreview->GetDiffuse();
  FLOATVECTOR4 cSpecular = m_pQLightPreview->GetSpecular();
  FLOATVECTOR3 vLightDir = m_pQLightPreview->GetLightDir();

  cAmbient[3] = horizontalSlider_ambientIntensity->value()/100.0f;
  cDiffuse[3] = horizontalSlider_diffuseIntensity->value()/100.0f;
  cSpecular[3] = horizontalSlider_specularIntensity->value()/100.0f;

  m_pQLightPreview->SetData(cAmbient,cDiffuse,cSpecular,vLightDir);
  m_pActiveRenderWin->SetLightColors(cAmbient, cDiffuse,
                                     cSpecular, vLightDir);
}

void MainWindow::SetTagVolume() {
  if (!m_pActiveRenderWin) return;
  if (checkBox_TagVolume->isChecked())
    m_pActiveRenderWin->SetInterpolant(tuvok::NearestNeighbor);
  else
    m_pActiveRenderWin->SetInterpolant(tuvok::Linear);
}

void MainWindow::UpdateInterpolant() {
  if (!m_pActiveRenderWin) return;
  checkBox_TagVolume->setChecked( m_pActiveRenderWin->GetInterpolant() == tuvok::NearestNeighbor);
}

void MainWindow::UpdateTFScaleSliders() {
  if (!m_pActiveRenderWin) return;

  verticalSlider_1DTransHistScale->setValue(int(m_pActiveRenderWin->GetCurrent1DHistScale()*verticalSlider_1DTransHistScale->maximum()));
  verticalSlider_2DTransHistScale->setValue(int(m_pActiveRenderWin->GetCurrent2DHistScale()*verticalSlider_2DTransHistScale->maximum()));
}

void MainWindow::UpdateColorWidget() {
  FLOATVECTOR4 cAmbient(1,1,1,0.1f);
  FLOATVECTOR4 cDiffuse(1,1,1,1.0f);
  FLOATVECTOR4 cSpecular(1,1,1,1);
  FLOATVECTOR3 vLightDir(0,0,-1);

  if (m_pActiveRenderWin && m_pActiveRenderWin->IsRendererValid()) {
    cAmbient  = m_pActiveRenderWin->GetAmbient();
    cDiffuse  = m_pActiveRenderWin->GetDiffuse();
    cSpecular = m_pActiveRenderWin->GetSpecular();
    vLightDir = m_pActiveRenderWin->GetLightDir();
  }

  m_pQLightPreview->SetData(cAmbient,cDiffuse,cSpecular,vLightDir);
  // these should probably be a size_t or even unsigned char, but Qt's APIs
  // take an int.
  const int intensity[3] = {
    static_cast<int>(cAmbient[3] * 100),
    static_cast<int>(cDiffuse[3] * 100),
    static_cast<int>(cSpecular[3] * 100)
  };
  horizontalSlider_ambientIntensity->setValue(intensity[0]);
  horizontalSlider_diffuseIntensity->setValue(intensity[1]);
  horizontalSlider_specularIntensity->setValue(intensity[2]);
}


void MainWindow::ResetRenderingParameters() {
  if (!m_pActiveRenderWin) return;
  m_pActiveRenderWin->ResetRenderingParameters();
}


static void CallQTProcessEvents()
{
  QApplication::processEvents();
}

void MainWindow::LuaSetIsoValueFloat(float fValue)
{
  SetIsoValue(fValue);
}

void MainWindow::LuaSetIsoValueInteger(int iValue)
{
  SetIsoValue(iValue);
}

void MainWindow::LuaMoveProgramWindow(const INTVECTOR2& pos) {
  move(pos.x, pos.y);
}

void MainWindow::LuaResizeProgramWindow(const UINTVECTOR2& size) {
  resize(size.x, size.y);
}

void MainWindow::RegisterLuaClasses() {
  shared_ptr<LuaScripting> ss(m_MasterController.LuaScript());
  string prefix = "iv3d.";

  ss->registerClass<RenderWindow>(
      this, &MainWindow::LuaCreateNewWindow,
      prefix + "renderer", "Constructs a new render window",
      LuaClassRegCallback<RenderWindow>::Type(
          &RenderWindow::RegisterLuaFunctions));

  ss->registerClass<RenderWindow>(
      this, &MainWindow::LuaLoadDatasetInternal, 
      prefix + "rendererWithParams",
      "Open a render window the additional parameters and a list of files.",
      LuaClassRegCallback<RenderWindow>::Type(
                &RenderWindow::RegisterLuaFunctions));

  // And some lua functions...

  // We need to expose QApplication::processEvents() because we are running
  // in the GUI thread... This is useful for scripts that take a long time to
  // run and need the UI to update.
  ss->registerFunction(
      &CallQTProcessEvents,
      prefix + "processUI",
      "Calls QT's QApplication::processEvents(). If you have a long running "
      "script and want the UI to update while the script is running, you will "
      "need to call this periodically. This is necessary because Tuvok runs on "
      "the GUI's thread instead of on its own thread.",
      false);

  m_MemReg.registerFunction(
    this, &MainWindow::LuaMoveProgramWindow, prefix + "move",
    "Moves the main window to position (x, y).", true);

  m_MemReg.registerFunction(
    this, &MainWindow::LuaResizeProgramWindow, prefix +  "resize",
    "Resizes the main window to size (w, h).", true);

  m_MemReg.registerFunction(this, &MainWindow::LuaPlaceActiveWindow, 
                            prefix + "placeActiveWindow",
                            "Moves the active window to specific coordinates.",
                            true);

  m_MemReg.registerFunction(this, &MainWindow::LuaResizeActiveWindow, 
                            prefix + "resizeActiveWindow",
                            "Resizes the active window.", true);

  m_MemReg.registerFunction(this, &MainWindow::ListSupportedImages, 
                            prefix + "listSupportedImages",
                            "Lists supported images.", false);

  m_MemReg.registerFunction(this, &MainWindow::ListSupportedVolumes, 
                            prefix + "listSupportedVolumes",
                            "Lists supported volumes.", false);

  m_MemReg.registerFunction(this, &MainWindow::ListSupportedGeometry, 
                            prefix + "listSupportedGeometry",
                            "Lists supported geometry.", false);

  m_MemReg.registerFunction(this, &MainWindow::SetStayOpen, 
                            prefix + "setStayOpen",
                            "Stay open after script ended "
                            "passed in via command line.", false);

  m_MemReg.registerFunction(this, &MainWindow::Use1DTrans, 
                            prefix + "use1DTFMode",
                            "Enable 1D transfer function mode.", false);

  m_MemReg.registerFunction(this, &MainWindow::Use2DTrans, 
                            prefix + "use2DTFMode",
                            "Enable 2D transfer function mode.", false);

  m_MemReg.registerFunction(this, &MainWindow::UseIso, 
                            prefix + "useIsoMode",
                            "Enable iso-surface rendering mode.", false);

  m_MemReg.registerFunction(this, &MainWindow::LuaSetIsoValueFloat, 
                            prefix + "setIsoValueFloat",
                            "Set threshold value for iso-surface rendering in the range [0.0, 1.0].", false);

  m_MemReg.registerFunction(this, &MainWindow::LuaSetIsoValueInteger, 
                            prefix + "setIsoValueInteger",
                            "Set threshold value for iso-surface rendering as integer.", false);

  std::shared_ptr<LuaScripting> reg = m_MasterController.LuaScript();
}

void MainWindow::closeMDISubWindowWithWidget(QWidget* widget) {
  QMdiSubWindow* foundWindow = nullptr;
  QList<QMdiSubWindow*> list = mdiArea->subWindowList();

  for (QList<QMdiSubWindow*>::iterator it = list.begin();
       it != list.end(); ++it) {
    if ((*it)->widget() == widget) {
      foundWindow = *it;
      break;
    }
  }

  if (foundWindow) {
    mdiArea->removeSubWindow(foundWindow);
  }
}

static std::string readfile(const std::wstring& filename) {
  std::ifstream ifs(SysTools::toNarrow(filename).c_str(), std::ios::in);
  ifs >> noskipws;
  return std::string(
    (std::istream_iterator<char>(ifs)),
    (std::istream_iterator<char>())
  );
}

bool MainWindow::RunLuaScript(const std::wstring& strFilename) {
  shared_ptr<LuaScripting> ss = m_MasterController.LuaScript();
  ss->setExpectedExceptionFlag(true);
  try {
    ss->exec(readfile(strFilename));
  } catch(const tuvok::LuaError& e) {
    T_ERROR("Lua error executing script: %s", e.what());
    return false;
  } catch(const std::exception& e) {
    T_ERROR("Error executing script: %s", e.what());
    return false;
  } catch(...) {
    T_ERROR("Unknown error executing script!");
    return false;
  }
  ss->setExpectedExceptionFlag(false);
  return true;
}

