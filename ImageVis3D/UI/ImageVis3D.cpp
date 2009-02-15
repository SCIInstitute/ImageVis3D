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

#include "ImageVis3D.h"
#include "BrowseData.h"
#include "FTPDialog.h"

#include <QtCore/QSettings>
#include <QtCore/QFileInfo>

#include <QtGui/QMdiSubWindow>
#include <QtGui/QCloseEvent>
#include <QtGui/QColorDialog>

#include <QtNetwork/QHttp>

#include "PleaseWait.h"

#include <fstream>
#include <iostream>
#include <string>
#include "../Tuvok/Basics/SysTools.h"
#include "../Tuvok/IO/IOManager.h"

#include "../IO/DialogConverter.h"


using namespace std;


MainWindow::MainWindow(MasterController& masterController,
           bool bScriptMode, /* = false */
           QWidget* parent /* = 0 */,
           Qt::WindowFlags flags /* = 0 */) :

  QMainWindow(parent, flags),
  m_pRedrawTimer(NULL),
  m_MasterController(masterController),
  m_strCurrentWorkspaceFilename(""),
  m_bShowVersionInTitle(true),
  m_bQuickopen(false),
  m_iMinFramerate(10),
  m_iLODDelay(1000),
  m_iActiveTS(500),
  m_iInactiveTS(100),
  m_pWelcomeDialog(new WelcomeDialog(this, Qt::Tool)),
  m_iBlendPrecisionMode(0),
  m_bPowerOfTwo(true),
  m_bDownSampleTo8Bits(false),
  m_bDisableBorder(false),
  m_bAvoidCompositing(false),
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
  m_bStayOpenAfterScriptEnd(false),
  m_pActiveRenderWin(false),
  m_pHttp(NULL),
  m_pUpdateFile(NULL),
  m_iHttpGetId(-1),
  m_bStartupCheck(false),
  m_bScriptMode(bScriptMode),
  m_pFTPDialog(NULL),
  m_strFTPTempFile(""),
  m_bFTPDeleteSource(true),
  m_bFTPFinished(true)
{
  RegisterCalls(m_MasterController.ScriptEngine());

  QCoreApplication::setOrganizationName("Scientific Computing and Imaging Institute, University of Utah");
  QCoreApplication::setOrganizationDomain("http://software.sci.utah.edu/");
  QCoreApplication::setApplicationName("ImageVis3D");
  QString qstrVersion = tr("%1").arg(IV3D_VERSION);
  QCoreApplication::setApplicationVersion(qstrVersion);

  setupUi(this);

  SetupWorkspaceMenu();

  if (!LoadDefaultGeometry()) SaveDefaultGeometry();

  if (!LoadDefaultWorkspace()) {
    InitAllWorkspaces();
    SaveDefaultWorkspace();
  }

  masterController.IOMan()->RegisterFinalConverter(new DialogConverter(this));

  UpdateMRUActions();
  UpdateMenus();

  m_pRedrawTimer = new QTimer(this);
  connect(m_pRedrawTimer, SIGNAL(timeout()), this, SLOT(CheckForRedraw()));
  m_pRedrawTimer->start(10);

  CheckSettings();
  ClearProgressView();

  connect(m_pWelcomeDialog, SIGNAL(CheckUpdatesClicked()),   this, SLOT(CheckForUpdates()));
  connect(m_pWelcomeDialog, SIGNAL(OnlineVideoTutClicked()), this, SLOT(OnlineVideoTut()));
  connect(m_pWelcomeDialog, SIGNAL(OnlineHelpClicked()),     this, SLOT(OnlineHelp()));
  connect(m_pWelcomeDialog, SIGNAL(OpenFromFileClicked()),   this, SLOT(LoadDataset()));
  connect(m_pWelcomeDialog, SIGNAL(OpenFromFileClicked(std::string)),   this, SLOT(LoadDataset(std::string)));
  connect(m_pWelcomeDialog, SIGNAL(OpenFromDirClicked()),    this, SLOT(LoadDirectory()));
  connect(m_pWelcomeDialog, SIGNAL(accepted()),              this, SLOT(CloseWelcome()));

  if (!m_bScriptMode && m_bShowWelcomeScreen) ShowWelcomeScreen();
  if (m_bCheckForUpdatesOnStartUp) QuietCheckForUpdates();
}

MainWindow::~MainWindow()
{
  if (m_pDebugOut == m_MasterController.DebugOut()) {
    m_MasterController.RemoveDebugOut(m_pDebugOut);
  }

  // cleanup updatefile, this codepath is taken for instance when the windows firewall blocked an http request
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
}


// ******************************************
// Filter Function Dock
// ******************************************


// ******************************************
// Render Mode
// ******************************************

void MainWindow::Use1DTrans() {
  if (!m_pActiveRenderWin) return;

  checkBox_Use2DTrans->setChecked(false);
  checkBox_Use2DTrans->setEnabled(true);
  checkBox_UseIso->setChecked(false);
  checkBox_UseIso->setEnabled(true);
  checkBox_Use1DTrans->setEnabled(false);
  checkBox_Use1DTrans->setChecked(true);
  radioButton_1DTrans->setChecked(true);

  checkBox_Lighting->setEnabled(true);

  toolButton_ExpMesh->setEnabled(false);
  m_1DTransferFunction->setEnabled(true);
  m_2DTransferFunction->setEnabled(false);

  if (m_pActiveRenderWin) m_pActiveRenderWin->SetRendermode(AbstrRenderer::RM_1DTRANS);
}


void MainWindow::Use2DTrans() {
  if (!m_pActiveRenderWin) return;

  checkBox_Use1DTrans->setChecked(false);
  checkBox_Use1DTrans->setEnabled(true);
  checkBox_UseIso->setChecked(false);
  checkBox_UseIso->setEnabled(true);
  checkBox_Use2DTrans->setEnabled(false);
  checkBox_Use2DTrans->setChecked(true);
  radioButton_2DTrans->setChecked(true);

  checkBox_Lighting->setEnabled(true);

  toolButton_ExpMesh->setEnabled(false);
  m_1DTransferFunction->setEnabled(false);
  m_2DTransferFunction->setEnabled(true);

  if (m_pActiveRenderWin) m_pActiveRenderWin->SetRendermode(AbstrRenderer::RM_2DTRANS);
}


void MainWindow::UseIso() {
  if (!m_pActiveRenderWin) return;

  checkBox_Use2DTrans->setChecked(false);
  checkBox_Use2DTrans->setEnabled(true);
  checkBox_Use1DTrans->setChecked(false);
  checkBox_Use1DTrans->setEnabled(true);
  checkBox_UseIso->setEnabled(false);
  checkBox_UseIso->setChecked(true);
  radioButton_Iso->setChecked(true);

  checkBox_Lighting->setEnabled(false);

  toolButton_ExpMesh->setEnabled(true);
  m_1DTransferFunction->setEnabled(false);
  m_2DTransferFunction->setEnabled(false);

  if (m_pActiveRenderWin) m_pActiveRenderWin->SetRendermode(AbstrRenderer::RM_ISOSURFACE);
}


void MainWindow::DisableAllTrans() {
  checkBox_Use2DTrans->setChecked(false);
  checkBox_Use2DTrans->setEnabled(false);
  checkBox_UseIso->setChecked(false);
  checkBox_UseIso->setEnabled(false);
  checkBox_Use1DTrans->setEnabled(false);
  checkBox_Use1DTrans->setChecked(false);
  radioButton_1DTrans->setChecked(false);

  m_1DTransferFunction->setEnabled(false);
  m_2DTransferFunction->setEnabled(false);
}


// ******************************************
// Filtering
// ******************************************

void MainWindow::FilterImage() {

  if (!m_pActiveRenderWin) return;

  // Get theactive tab
  int tab = tabWidget_Filter->currentIndex();

  int var0 = 0;
  int var1 = 0;

  switch( tab ) {

  case 0: // Histogram
    var0 = spinBox_FilterHistoBins->value();
    var1 = spinBox_FilterHistoAlpha->value();
    break;

  case 1: // Median
    var0 = spinBox_FilterMedianRadius->value();
    break;

  case 2: // Gaussian
    var0 = spinBox_FilterGaussianVariance->value();
    var1 = spinBox_FilterGaussianKernel->value();
    break;

  case -1:
  default:
    return;
  }

  m_MasterController.DebugOut()->
    Message("MainWindow::FilterImage",
      "Performing filter %d on %s.", tab,
      m_pActiveRenderWin->GetDatasetName().toStdString().c_str() );

  QString fileName = m_pActiveRenderWin->GetDatasetName();

  /// \todo ARS -- this should return a pointer to memory.
  m_MasterController.Filter( m_pActiveRenderWin->GetDatasetName().toStdString(),
           tab,
           &var0, &var1 );

  RenderWindow *renderWin;
  if( radioButton_FilterUpdate->isChecked() ) {
    renderWin = CreateNewRenderWindow(fileName);
  } else { // if( radioButton_FilterCreate->isChecked() )
    m_pActiveRenderWin->GetQtWidget()->close();
    renderWin = CreateNewRenderWindow(fileName);
  }

  renderWin->GetQtWidget()->show();
  RenderWindowActive(renderWin);
}


void MainWindow::SetLighting(bool bLighting) {
  if (m_pActiveRenderWin != NULL) m_pActiveRenderWin->SetUseLighting(bLighting);
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

void MainWindow::SetIsoValue(float fValue) {
  if (m_pActiveRenderWin != NULL) {
    int iMaxSize = int(m_pActiveRenderWin->GetDynamicRange())-1;
    m_pActiveRenderWin->SetIsoValue(fValue);
    UpdateIsoValLabel(int(fValue*iMaxSize), iMaxSize);
  }
}

void MainWindow::SetIsoValue(int iValue) {
  if (m_pActiveRenderWin != NULL) {
    int iMaxSize = int(m_pActiveRenderWin->GetDynamicRange()-1);
    m_pActiveRenderWin->SetIsoValue(float(iValue)/float(iMaxSize));
    UpdateIsoValLabel(iValue, iMaxSize);
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
  int iMaxSize = int(m_pActiveRenderWin->GetDynamicRange()-1);
  if (m_pActiveRenderWin != NULL) m_pActiveRenderWin->SetCVIsoValue(float(iValue)/float(iMaxSize));
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

void MainWindow::ToggleLocalBBox(bool bRenderBBox)
{
  if (m_pActiveRenderWin != NULL) m_pActiveRenderWin->SetLocalBBox(bRenderBBox);
}

void MainWindow::ToggleClipPlane(bool bClip)
{
  m_MasterController.DebugOut()->Message("MainWindow::ToggleClipPlane",
                                         "clip %d", static_cast<int>(bClip));
  AbstrRenderer *ren = m_pActiveRenderWin->GetRenderer();
  if(bClip && ren) {
    m_pActiveRenderWin->GetRenderer()->EnableClipPlane();
  } else {
    m_pActiveRenderWin->GetRenderer()->DisableClipPlane();
  }
}

void MainWindow::closeEvent(QCloseEvent *event)
{
  if (m_bAutoSaveGEO) SaveDefaultGeometry();
  if (m_bAutoSaveWSP) SaveDefaultWorkspace();
  m_MasterController.RemoveDebugOut(m_pDebugOut);
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
#ifdef TUVOK_OS_LINUX
  QCoreApplication::processEvents();
#endif
  for (int i = 0;i<mdiArea->subWindowList().size();i++) {
    QWidget* w = mdiArea->subWindowList().at(i)->widget();
    RenderWindow* subwindow = WidgetToRenderWin(w);
    if (subwindow == m_pActiveRenderWin) return mdiArea->subWindowList().at(i);
  }
  return NULL;
}

RenderWindow* MainWindow::ActiveRenderWin() {
#ifdef TUVOK_OS_LINUX
  QCoreApplication::processEvents();
#endif
  if (mdiArea->activeSubWindow())
    return WidgetToRenderWin(mdiArea->activeSubWindow()->widget());
  else
    return NULL;
}


RenderWindow* MainWindow::WidgetToRenderWin(QWidget* w) {
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
