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
#include "ImageVis3D.h"
#include "BrowseData.h"
#include "FTPDialog.h"

#include <QtCore/QSettings>
#include <QtCore/QFileInfo>
#include <QtCore/QTimer>
#include <QtGui/QMessageBox>

#include <QtGui/QMdiSubWindow>
#include <QtGui/QCloseEvent>
#include <QtGui/QColorDialog>

#include <QtNetwork/QHttp>

#include "PleaseWait.h"
#include "CrashDetDlg.h"

#include <fstream>
#include <iostream>
#include <string>
#include "../Tuvok/Basics/SystemInfo.h"
#include "../Tuvok/Basics/SysTools.h"
#include "../Tuvok/IO/IOManager.h"
#include "../Tuvok/Basics/MathTools.h"
#include "../IO/DialogConverter.h"

using namespace std;

static const int IV3D_TIMER_INTERVAL = 20; // in milliseconds

MainWindow::MainWindow(MasterController& masterController,
           bool bScriptMode, /* = false */
           QWidget* parent /* = 0 */,
           Qt::WindowFlags flags /* = 0 */) :
  QMainWindow(parent, flags),
  m_pRedrawTimer(NULL),
  m_MasterController(masterController),
  m_strCurrentWorkspaceFilename(""),
  m_strTempDir("."),  // changed in the constructor
  m_bShowVersionInTitle(true),
  m_bQuickopen(true),
  m_iMinFramerate(10),
  m_iLODDelay(1000),
  m_bUseAllMeans(false),
  m_iActiveTS(500),
  m_iInactiveTS(100),
  m_bWriteLogFile(false),
  m_strLogFileName("debugLog.txt"),
  m_iLogLevel(2),
  m_pWelcomeDialog(new WelcomeDialog(this, Qt::Tool)),
  m_pMetadataDialog(new MetadataDlg(this, Qt::Tool)),
  m_iBlendPrecisionMode(0),
  m_bPowerOfTwo(true),
  m_bDownSampleTo8Bits(false),
  m_bDisableBorder(false),
  m_bNoRCClipplanes(false),
  m_bI3MFeatures(false),
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
  m_pTextout(NULL),
  m_pActiveRenderWin(NULL),
  m_pHttp(NULL),
  m_pUpdateFile(NULL),
  m_iHttpGetId(-1),
  m_bStartupCheck(false),
  m_bScriptMode(bScriptMode),
  m_pFTPDialog(NULL),
  m_strFTPTempFile(""),
  m_bFTPDeleteSource(true),
  m_bFTPFinished(true),
  m_bClipDisplay(true),
  m_bClipLocked(false)
{
  RegisterCalls(m_MasterController.ScriptEngine());

  QCoreApplication::setOrganizationName("Scientific Computing and Imaging Institute, University of Utah");
  QCoreApplication::setOrganizationDomain(SCI_ORGANIZATION_DOMAIN);
  QCoreApplication::setApplicationName("ImageVis3D");
  QString qstrVersion = tr("%1").arg(IV3D_VERSION);
  QCoreApplication::setApplicationVersion(qstrVersion);

  QString path = QApplication::applicationDirPath();
  masterController.SysInfo()->SetProgramPath(string(path.toAscii()));

  setupUi(this);

  SysTools::GetTempDirectory(m_strTempDir);

  SetupWorkspaceMenu();

  if (!LoadDefaultGeometry()) SaveDefaultGeometry();

  if (!LoadDefaultWorkspace()) {
    InitAllWorkspaces();
    SaveDefaultWorkspace();
  }

  this->show();
  SetAndCheckRunningFlag();

  masterController.IOMan()->RegisterFinalConverter(new DialogConverter(this));

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
  connect(m_pWelcomeDialog, SIGNAL(OpenFromFileClicked(std::string)),   this, SLOT(LoadDataset(std::string)));
  connect(m_pWelcomeDialog, SIGNAL(OpenFromDirClicked()),    this, SLOT(LoadDirectory()));
  connect(m_pWelcomeDialog, SIGNAL(accepted()),              this, SLOT(CloseWelcome()));

  if (!m_bScriptMode && m_bCheckForUpdatesOnStartUp) QuietCheckForUpdates();
  if (!m_bScriptMode && m_bShowWelcomeScreen) ShowWelcomeScreen();

  checkBox_ClipShow->setEnabled(false);
  checkBox_ClipLockObject->setEnabled(false);
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

  // cleanup updatefile, this codepath is taken for instance when the
  // windows firewall blocked an http request
  if (m_pUpdateFile && m_pUpdateFile->isOpen()) {
    m_pUpdateFile->close();
    m_pUpdateFile->remove();
    delete m_pUpdateFile;
    m_pUpdateFile = NULL;
  }

  delete m_pHttp;
  delete m_pFTPDialog;
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
  UINT32 iInstanceCounter = settings.value("InstanceCounter", 0).toUInt();
  UINT32 iSaneCounter = settings.value("SaneCounter", 0).toUInt();

  settings.beginGroup("Performance");
  bool bWriteLogFile = settings.value("WriteLogFile", m_bWriteLogFile).toBool();
  bool bShowCrashDialog = settings.value("ShowCrashDialog", true).toBool();
  QString strLogFileName = settings.value("LogFileName", m_strLogFileName).toString();
  settings.endGroup();

 if (iInstanceCounter) {
    if (bShowCrashDialog) {
      settings.setValue("SaneCounter", 0);
      CrashDetDlg* d = NULL;
      if (bWriteLogFile && SysTools::FileExists(string(strLogFileName.toAscii()))) {
        d = new CrashDetDlg("Crash recovery", "Either ImageVis3D crashed or it is currently running in a second process. If it crashed do you want to submit the logfile?", false, this);
        if (d->exec() == QDialog::Accepted) {
          ReportABug(string(strLogFileName.toAscii()));
          remove(strLogFileName.toAscii());
        }
      } else {
        if (!bWriteLogFile) {
          d = new CrashDetDlg("Crash recovery", "Either ImageVis3D crashed or it is currently running in a second process. If it crashed do you want to enable debugging?", false, this);
          if (d->exec() == QDialog::Accepted) {
            settings.setValue("Performance/WriteLogFile", true);
            settings.setValue("Performance/LogLevel", 2);
          } else {
            // if debuging was not enabled assume that has been opended multiple times
            settings.setValue("SaneCounter", iSaneCounter);
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

  settings.setValue("InstanceCounter", iInstanceCounter+1);
}

void MainWindow::RemoveRunningFlag() {
  QSettings settings;
//  UINT32 iInstanceCounter = settings.value("InstanceCounter", 1).toUInt();
  settings.setValue("InstanceCounter", 0);

  UINT32 iSaneCounter = settings.value("SaneCounter", 0).toUInt();
  settings.setValue("SaneCounter", iSaneCounter+1);
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
  desc = tr("Sampling Rate (%1%)").arg(iValue);
  label_SamplingRate->setText(desc);
}

void MainWindow::SetSampleRateSlider(int iValue) {
  horizontalSlider_Sampling->setValue(iValue);
  UpdateSampleRateLabel(iValue);
}

// Script command to update isovalue.
void MainWindow::SetIsoValue(float fValue) {
  if (m_pActiveRenderWin != NULL) {
    int iMaxSize = int(m_pActiveRenderWin->GetDynamicRange().second)-1;
    fValue = MathTools::Clamp(fValue, 0.0f, 1.0f);
    m_pActiveRenderWin->SetIsoValue(fValue * iMaxSize);
    UpdateIsoValLabel(int(fValue * iMaxSize), iMaxSize);
  }
}

// UI command to update isovalue.
void MainWindow::SetIsoValue(int iValue) {
  if (m_pActiveRenderWin != NULL) {
    m_pActiveRenderWin->SetIsoValue(float(iValue));
    int iMaxSize = int(m_pActiveRenderWin->GetDynamicRange().second-1);
    UpdateIsoValLabel(iValue, iMaxSize);
  }
}

// Script command to update focus isovalue
void MainWindow::SetClearViewIsoValue(float fValue) {
  if (m_pActiveRenderWin != NULL) {
    int iMaxSize = int(m_pActiveRenderWin->GetDynamicRange().second)-1;
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
  int iMaxSize = int(m_pActiveRenderWin->GetDynamicRange().second-1);
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
  if(bClip) {
    checkBox_ClipShow->setEnabled(true);
    checkBox_ClipLockObject->setEnabled(true);
  } else {
    checkBox_ClipShow->setEnabled(false);
    checkBox_ClipLockObject->setEnabled(false);
  }
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
  int iMaxVal = static_cast<int>(
    m_pActiveRenderWin->GetRenderer()->GetDataset().GetNumberOfTimesteps()
  );
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
    FLOATVECTOR3 vIsoColor = m_pActiveRenderWin->GetIsosufaceColor();
    QColor color = QColorDialog::getColor(qRgba(int(vIsoColor.x*255),
                                                int(vIsoColor.y*255),
                                                int(vIsoColor.z*255),
                                                255),
                                                this);
    if (color.isValid()) {

      vIsoColor = FLOATVECTOR3(color.red() / 255.0f, color.green() / 255.0f, color.blue() / 255.0f);
      m_pActiveRenderWin->SetIsosufaceColor(vIsoColor);
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
    m_pActiveRenderWin->GetRenderer()->SetColors(cAmbient, cDiffuse,
                                                 cSpecular, vLightDir);
  }
}


void MainWindow::LightMoved() {
  if (!m_pActiveRenderWin) return;

  FLOATVECTOR4 cAmbient  = m_pQLightPreview->GetAmbient();
  FLOATVECTOR4 cDiffuse  = m_pQLightPreview->GetDiffuse();
  FLOATVECTOR4 cSpecular = m_pQLightPreview->GetSpecular();
  FLOATVECTOR3 vLightDir = m_pQLightPreview->GetLightDir();

  m_pActiveRenderWin->GetRenderer()->SetColors(cAmbient, cDiffuse,
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
  m_pActiveRenderWin->GetRenderer()->SetColors(cAmbient, cDiffuse,
                                               cSpecular, vLightDir);
}


void MainWindow::UpdateColorWidget() {
  FLOATVECTOR4 cAmbient(1,1,1,0.2f);
  FLOATVECTOR4 cDiffuse(1,1,1,0.8f);
  FLOATVECTOR4 cSpecular(1,1,1,1);
  FLOATVECTOR3 vLightDir(0,0,-1);

  if (m_pActiveRenderWin && m_pActiveRenderWin->GetRenderer()) {
    cAmbient  = m_pActiveRenderWin->GetRenderer()->GetAmbient();
    cDiffuse  = m_pActiveRenderWin->GetRenderer()->GetDiffuse();
    cSpecular = m_pActiveRenderWin->GetRenderer()->GetSpecular();
    vLightDir = m_pActiveRenderWin->GetRenderer()->GetLightDir();
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
