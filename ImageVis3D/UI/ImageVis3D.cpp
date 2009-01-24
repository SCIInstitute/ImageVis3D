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

#include <QtCore/QTimer>
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
#include "../Tuvok/DebugOut/MultiplexOut.h"

#include "../IO/DialogConverter.h"


using namespace std;


MainWindow::MainWindow(MasterController& masterController,
           bool bScriptMode, /* = false */
           QWidget* parent /* = 0 */,
           Qt::WindowFlags flags /* = 0 */) :

  QMainWindow(parent, flags),
  m_MasterController(masterController),
  m_strCurrentWorkspaceFilename(""),
  m_bShowVersionInTitle(true),
  m_bQuickopen(false),
  m_iMinFramerate(10),
  m_iLODDelay(1000),
  m_iActiveTS(500),
  m_iInactiveTS(100),
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
  m_bStayOpenAfterScriptEnd(false),
  m_pHttp(NULL),
  m_pUpdateFile(NULL),
  m_iHttpGetId(-1),
  m_bStartupCheck(false),
  m_bScriptMode(bScriptMode),
  m_pDialog(NULL),
  m_pTempFile(NULL)
{
  RegisterCalls(m_MasterController.ScriptEngine());

  QCoreApplication::setOrganizationName("Scientific Computing and Imaging Institute, University of Utah");
  QCoreApplication::setOrganizationDomain("http://software.sci.utah.edu/");
  QCoreApplication::setApplicationName("ImageVis3D");
  QString qstrVersion = tr("%1").arg(IV3D_VERSION);
  QCoreApplication::setApplicationVersion(qstrVersion);

  setupUi(this);

  SetupWorkspaceMenu();

  if (!LoadGeometry("Default.geo", true)) {
    SaveGeometry("Default.geo");
  }

  if (!LoadWorkspace("Default.wsp", true)) {
    InitAllWorkspaces();
    SaveWorkspace("Default.wsp");
  }

  masterController.IOMan()->RegisterFinalConverter(new DialogConverter(this));
  
  UpdateMRUActions();
  UpdateMenus();

  QTimer *timer = new QTimer(this);
  connect(timer, SIGNAL(timeout()), this, SLOT(CheckForRedraw()));
  timer->start(10);

  CheckSettings();
  ClearProgressView();
  
  if (m_bCheckForUpdatesOnStartUp) QuietCheckForUpdates();

}

MainWindow::~MainWindow()
{
  if (m_DebugOut == m_MasterController.DebugOut()) {
    m_MasterController.RemoveDebugOut(m_DebugOut);
  } else {
    // if the debugger was replaced by a multiplexer (for instance for file logging) remove it from the multiplexer
    MultiplexOut* p = dynamic_cast<MultiplexOut*>(m_MasterController.DebugOut());
    if (p != NULL) p->RemoveDebugOut(m_DebugOut);      
  }
  delete m_DebugOut;

  // cleanup updatefile, this codepath is taken for instance when the windows firewall blocked an http request
  if (m_pUpdateFile && m_pUpdateFile->isOpen()) {
    m_pUpdateFile->close();
    m_pUpdateFile->remove();
    delete m_pUpdateFile;
    m_pUpdateFile = NULL;
  }

  delete m_pHttp;
  delete m_pDialog;
}


// ******************************************
// Filter Function Dock
// ******************************************


// ******************************************
// Render Mode
// ******************************************

void MainWindow::Use1DTrans() {
  if (!ActiveRenderWin()) return;

  checkBox_Use2DTrans->setChecked(false);
  checkBox_Use2DTrans->setEnabled(true);
  checkBox_UseIso->setChecked(false);
  checkBox_UseIso->setEnabled(true);
  checkBox_Use1DTrans->setEnabled(false);
  checkBox_Use1DTrans->setChecked(true);
  radioButton_1DTrans->setChecked(true);

  checkBox_Lighting->setEnabled(true);

  m_1DTransferFunction->setEnabled(true);
  m_2DTransferFunction->setEnabled(false);

  if (ActiveRenderWin()) ActiveRenderWin()->SetRendermode(AbstrRenderer::RM_1DTRANS);
}


void MainWindow::Use2DTrans() {
  if (!ActiveRenderWin()) return;

  checkBox_Use1DTrans->setChecked(false);
  checkBox_Use1DTrans->setEnabled(true);
  checkBox_UseIso->setChecked(false);
  checkBox_UseIso->setEnabled(true);
  checkBox_Use2DTrans->setEnabled(false);
  checkBox_Use2DTrans->setChecked(true);
  radioButton_2DTrans->setChecked(true);

  checkBox_Lighting->setEnabled(true);

  m_1DTransferFunction->setEnabled(false);
  m_2DTransferFunction->setEnabled(true);

  if (ActiveRenderWin()) ActiveRenderWin()->SetRendermode(AbstrRenderer::RM_2DTRANS);
}


void MainWindow::UseIso() {
  if (!ActiveRenderWin()) return;

  checkBox_Use2DTrans->setChecked(false);
  checkBox_Use2DTrans->setEnabled(true);
  checkBox_Use1DTrans->setChecked(false);
  checkBox_Use1DTrans->setEnabled(true);
  checkBox_UseIso->setEnabled(false);
  checkBox_UseIso->setChecked(true);
  radioButton_Iso->setChecked(true);

  checkBox_Lighting->setEnabled(false);

  m_1DTransferFunction->setEnabled(false);
  m_2DTransferFunction->setEnabled(false);

  if (ActiveRenderWin()) ActiveRenderWin()->SetRendermode(AbstrRenderer::RM_ISOSURFACE);
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

  if (!ActiveRenderWin()) return;

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
      ActiveRenderWin()->GetDatasetName().toStdString().c_str() );

  QString fileName = ActiveRenderWin()->GetDatasetName();

  /// \todo ARS -- this should return a pointer to memory.
  m_MasterController.Filter( ActiveRenderWin()->GetDatasetName().toStdString(),
           tab,
           &var0, &var1 );

  RenderWindow *renderWin;
  if( radioButton_FilterUpdate->isChecked() ) {
    renderWin = CreateNewRenderWindow(fileName);
  } else { // if( radioButton_FilterCreate->isChecked() )
    ActiveRenderWin()->GetQtWidget()->close();
    renderWin = CreateNewRenderWindow(fileName);
  }

  renderWin->GetQtWidget()->show();
  RenderWindowActive(renderWin);
}


void MainWindow::SetLighting(bool bLighting) {
  if (ActiveRenderWin() != NULL) ActiveRenderWin()->SetUseLighting(bLighting);
}

void MainWindow::SetSampleRate(int iValue) {
  if (ActiveRenderWin() != NULL) ActiveRenderWin()->SetSampleRateModifier(iValue/100.0f); 
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
  if (ActiveRenderWin() != NULL) {
    int iMaxSize = int(ActiveRenderWin()->GetDynamicRange());
    ActiveRenderWin()->SetIsoValue(fValue);
    UpdateIsoValLabel(int(fValue*iMaxSize), iMaxSize);
  }
}

void MainWindow::SetIsoValue(int iValue) {
  if (ActiveRenderWin() != NULL) {
    int iMaxSize = int(ActiveRenderWin()->GetDynamicRange());
    ActiveRenderWin()->SetIsoValue(float(iValue)/float(iMaxSize));
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
  int iMaxSize = int(ActiveRenderWin()->GetDynamicRange());
  if (ActiveRenderWin() != NULL) ActiveRenderWin()->SetCVIsoValue(float(iValue)/float(iMaxSize));
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
  if (ActiveRenderWin() != NULL) ActiveRenderWin()->SetCVSize(float(99-iValue)/9.9f);
}

void MainWindow::SetContextScale(int iValue) {
  if (ActiveRenderWin() != NULL) ActiveRenderWin()->SetCVContextScale(float(iValue)/10.0f);
}

void MainWindow::SetBorderSize(int iValue) {
  if (ActiveRenderWin() != NULL) ActiveRenderWin()->SetCVBorderScale(float(99-iValue));
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
  if (ActiveRenderWin() != NULL) ActiveRenderWin()->SetGlobalBBox(bRenderBBox);
}

void MainWindow::ToggleLocalBBox(bool bRenderBBox)
{
  if (ActiveRenderWin() != NULL) ActiveRenderWin()->SetLocalBBox(bRenderBBox);
}


void MainWindow::closeEvent(QCloseEvent *event)
{
  if (m_bAutoSaveGEO) SaveGeometry("Default.geo");
  if (m_bAutoSaveWSP) SaveWorkspace("Default.wsp");
  event->accept();
}


void MainWindow::ChooseIsoColor()
{
  if (ActiveRenderWin())  {
    FLOATVECTOR3 vIsoColor = ActiveRenderWin()->GetIsosufaceColor();
    QColor color = QColorDialog::getColor(qRgba(int(vIsoColor.x*255),
                                                int(vIsoColor.y*255),
                                                int(vIsoColor.z*255),
                                                255),
                                                this);
    if (color.isValid()) {

      vIsoColor = FLOATVECTOR3(color.red() / 255.0f, color.green() / 255.0f, color.blue() / 255.0f);
      ActiveRenderWin()->SetIsosufaceColor(vIsoColor);
    }
  }
}

void MainWindow::ChooseFocusColor()
{
  if (ActiveRenderWin())  {
    FLOATVECTOR3 vIsoColor = ActiveRenderWin()->GetCVColor();
    QColor color = QColorDialog::getColor(qRgba(int(vIsoColor.x*255),
                                                int(vIsoColor.y*255),
                                                int(vIsoColor.z*255),
                                                255),
                                                this);
    if (color.isValid()) {

      vIsoColor = FLOATVECTOR3(color.red() / 255.0f, color.green() / 255.0f, color.blue() / 255.0f);
      ActiveRenderWin()->SetCVColor(vIsoColor);
    }
  }
}

void MainWindow::ToggleClearView() {
  if (ActiveRenderWin()) {
    ActiveRenderWin()->SetCV(checkBox_ClearView->isChecked());
    frame_ClearView->setEnabled(checkBox_ClearView->isChecked());
  }
}

// ******************************************
// Dataset interaction
// ******************************************

void MainWindow::RotateCurrentViewX(double angle) {
  if (ActiveRenderWin()) {
    FLOATMATRIX4 matRot;
    matRot.RotationX(3.141592653589793238462643383*angle/180.0);
    ActiveRenderWin()->Rotate(matRot);
  }
} 

void MainWindow::RotateCurrentViewY(double angle) {
  if (ActiveRenderWin()) {
    FLOATMATRIX4 matRot;
    matRot.RotationY(3.141592653589793238462643383*angle/180.0);
    ActiveRenderWin()->Rotate(matRot);
  }
} 

void MainWindow::RotateCurrentViewZ(double angle) {
  if (ActiveRenderWin()) {
    FLOATMATRIX4 matRot;
    matRot.RotationZ(3.141592653589793238462643383*angle/180.0);
    ActiveRenderWin()->Rotate(matRot);
  }
}

void MainWindow::TranslateCurrentView(double x, double y, double z) {
  if (ActiveRenderWin()) {
    FLOATMATRIX4 matTrans;
    matTrans.Translation(x,y,z);
    ActiveRenderWin()->Translate(matTrans);
  }
}


void MainWindow::ToggleFullscreen() {
  if (ActiveRenderWin())
    ActiveRenderWin()->ToggleFullscreen();
}


RenderWindow* MainWindow::ActiveRenderWin() {
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
