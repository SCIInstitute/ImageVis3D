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

#include <QtCore/QTimer>
#include <QtGui/QMdiSubWindow>
#include <QtCore/QSettings>
#include <QtGui/QCloseEvent>
#include <QtGui/QColorDialog>

#include "PleaseWait.h"

#include <fstream>
#include <iostream>
#include <string>
#include <Basics/SysTools.h>

#include <DebugOut/MultiplexOut.h>


using namespace std;


MainWindow::MainWindow(MasterController& masterController,
		       QWidget* parent /* = 0 */,
		       Qt::WindowFlags flags /* = 0 */) :

  QMainWindow(parent, flags),
  m_MasterController(masterController),
  m_strCurrentWorkspaceFilename(""),
  m_ActiveRenderWin(NULL),
  m_bQuickopen(false),
  m_iMinFramerate(10),
  m_iLODDelay(1000),
  m_iActiveTS(500),
  m_iInactiveTS(100),
  m_iBlendPrecisionMode(0),
  m_bPowerOfTwo(false),
  m_bAvoidCompositing(false),
  m_bAutoSaveGEO(true),
  m_bAutoSaveWSP(true),
  m_eVolumeRendererType(MasterController::OPENGL_SBVR),
  m_bUpdatingLockView(false),
  m_strLogoFilename(""),
  m_iLogoPos(3),
  m_bAutoLockClonedWindow(false),
  m_bAbsoluteViewLocks(true)
{
  QCoreApplication::setOrganizationName("Scientific Computing and Imaging Institute, University of Utah");
  QCoreApplication::setOrganizationDomain("http://software.sci.utah.edu/");
  QCoreApplication::setApplicationName("ImageVis3D");
  QCoreApplication::setApplicationVersion(IV3D_VERSION);

  setupUi(this);

  SetupWorkspaceMenu();

  if (!LoadGeometry("Default.geo", true)) {
    SaveGeometry("Default.geo");
  }

  if (!LoadWorkspace("Default.wsp", true)) {
    InitAllWorkspaces();
    SaveWorkspace("Default.wsp");
  }
  
  UpdateMRUActions();
  UpdateMenus();

  QTimer *timer = new QTimer(this);
  connect(timer, SIGNAL(timeout()), this, SLOT(CheckForRedraw()));
  timer->start(10);

  CheckSettings();
  ClearProgressView();
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
}


// ******************************************
// Filter Function Dock
// ******************************************


// ******************************************
// Render Mode
// ******************************************

void MainWindow::Use1DTrans() {
  if (!m_ActiveRenderWin) return;

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

  if (m_ActiveRenderWin) m_ActiveRenderWin->SetRendermode(AbstrRenderer::RM_1DTRANS);
}


void MainWindow::Use2DTrans() {
  if (!m_ActiveRenderWin) return;

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

  if (m_ActiveRenderWin) m_ActiveRenderWin->SetRendermode(AbstrRenderer::RM_2DTRANS);
}


void MainWindow::UseIso() {
  if (!m_ActiveRenderWin) return;

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

  if (m_ActiveRenderWin) m_ActiveRenderWin->SetRendermode(AbstrRenderer::RM_ISOSURFACE);
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

  if (!m_ActiveRenderWin) return;

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
	    m_ActiveRenderWin->GetDatasetName().toStdString().c_str() );

  QString fileName = m_ActiveRenderWin->GetDatasetName();

  /// \todo ARS -- this should return a pointer to memory.
  m_MasterController.Filter( m_ActiveRenderWin->GetDatasetName().toStdString(),
			     tab,
			     &var0, &var1 );


  /// \todo ARS -- Need to be able to a CreateNewRenderWindow based on memory
  RenderWindow *renderWin;
  if( radioButton_FilterUpdate->isChecked() ) {
    renderWin = CreateNewRenderWindow(fileName);
    renderWin->show();
  } else { // if( radioButton_FilterCreate->isChecked() )
    m_ActiveRenderWin->close();
    renderWin = CreateNewRenderWindow(fileName);
    renderWin->show();
  }
}


void MainWindow::SetLighting(bool bLighting) {
  RenderWindow* w = GetActiveRenderWindow();
  if (w != NULL) {
    w->SetUseLigthing(bLighting);
  }
}

void MainWindow::SetSampleRate(int iValue) {
  if (m_ActiveRenderWin != NULL) m_ActiveRenderWin->SetSampleRateModifier(iValue/100.0f); 
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

void MainWindow::SetIsoValue(int iValue) {
  int iMaxSize = int(m_ActiveRenderWin->GetDynamicRange().x);
  if (m_ActiveRenderWin != NULL) m_ActiveRenderWin->SetIsoValue(float(iValue)/float(iMaxSize));
  UpdateIsoValLabel(iValue, iMaxSize);
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
  int iMaxSize = int(m_ActiveRenderWin->GetDynamicRange().x);
  if (m_ActiveRenderWin != NULL) m_ActiveRenderWin->SetCVIsoValue(float(iValue)/float(iMaxSize));
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
  if (m_ActiveRenderWin != NULL) m_ActiveRenderWin->SetCVSize(float(99-iValue)/9.9f);
}

void MainWindow::SetContextScale(int iValue) {
  if (m_ActiveRenderWin != NULL) m_ActiveRenderWin->SetCVContextScale(float(iValue)/10.0f);
}

void MainWindow::SetBorderSize(int iValue) {
  if (m_ActiveRenderWin != NULL) m_ActiveRenderWin->SetCVBorderScale(float(99-iValue));
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
  if (m_ActiveRenderWin != NULL) m_ActiveRenderWin->SetGlobalBBox(bRenderBBox);
}

void MainWindow::ToggleLocalBBox(bool bRenderBBox)
{
  if (m_ActiveRenderWin != NULL) m_ActiveRenderWin->SetLocalBBox(bRenderBBox);
}


void MainWindow::closeEvent(QCloseEvent *event)
{
  if (m_bAutoSaveGEO) SaveGeometry("Default.geo");
  if (m_bAutoSaveWSP) SaveWorkspace("Default.wsp");
  event->accept();
}


void MainWindow::ChooseIsoColor()
{
  if (m_ActiveRenderWin)  {
    FLOATVECTOR3 vIsoColor = m_ActiveRenderWin->GetIsosufaceColor();
    QColor color = QColorDialog::getColor(qRgba(int(vIsoColor.x*255),
                                                int(vIsoColor.y*255),
                                                int(vIsoColor.z*255),
                                                255),
                                                this);
    if (color.isValid()) {

      vIsoColor = FLOATVECTOR3(color.red() / 255.0f, color.green() / 255.0f, color.blue() / 255.0f);
      m_ActiveRenderWin->SetIsosufaceColor(vIsoColor);
    }
  }
}

void MainWindow::ChooseFocusColor()
{
  if (m_ActiveRenderWin)  {
    FLOATVECTOR3 vIsoColor = m_ActiveRenderWin->GetCVColor();
    QColor color = QColorDialog::getColor(qRgba(int(vIsoColor.x*255),
                                                int(vIsoColor.y*255),
                                                int(vIsoColor.z*255),
                                                255),
                                                this);
    if (color.isValid()) {

      vIsoColor = FLOATVECTOR3(color.red() / 255.0f, color.green() / 255.0f, color.blue() / 255.0f);
      m_ActiveRenderWin->SetCVColor(vIsoColor);
    }
  }
}

void MainWindow::ToggleClearView() {
  if (m_ActiveRenderWin) {
    m_ActiveRenderWin->SetCV(checkBox_ClearView->isChecked());
    frame_ClearView->setEnabled(checkBox_ClearView->isChecked());
  }
}
