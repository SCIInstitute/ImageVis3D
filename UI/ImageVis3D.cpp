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
#include <QtGui/QFileDialog>
#include <QtCore/QSettings>
#include <QtGui/QMessageBox>
#include <QtGui/QInputDialog>
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
  m_ActiveRenderWin(NULL)
{
  QCoreApplication::setOrganizationName("Scientific Computing and Imaging Institute, University of Utah");
  QCoreApplication::setOrganizationDomain("http://software.sci.utah.edu/");
  QCoreApplication::setApplicationName("ImageVis3D");


  setupUi(this);

  SetupWorkspaceMenu();

  LoadGeometry("Default.geo", true);
  LoadWorkspace("Default.wsp", true);
  
  UpdateMRUActions();
  UpdateMenus();

  QTimer *timer = new QTimer(this);
  connect(timer, SIGNAL(timeout()), this, SLOT(CheckForRedraw()));
  timer->start(20);

  CheckSettings();
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

  m_1DTransferFunction->setEnabled(true);
  m_2DTransferFunction->setEnabled(false);
  // todo disable iso controlls

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

  m_1DTransferFunction->setEnabled(false);
  m_2DTransferFunction->setEnabled(true);
  // todo disable iso controls

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

  m_1DTransferFunction->setEnabled(false);
  m_2DTransferFunction->setEnabled(false);
  // todo enable iso controlls
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
  // todo disable iso controlls
}


// ******************************************
// Locks
// ******************************************
 
void MainWindow::EditViewLocks() {
  pushButton_RelativeLock->setEnabled(true);  
}

void MainWindow::EditRenderLocks() {
  pushButton_RelativeLock->setEnabled(false);
}

void MainWindow::EditToolsLocks() {
  pushButton_RelativeLock->setEnabled(false);
}

void MainWindow::EditFiltersLocks() {
  pushButton_RelativeLock->setEnabled(false);
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
  if( radioButton_FilterUpdate->isChecked() ) {

    RenderWindow *renderWin = CreateNewRenderWindow(fileName);
    renderWin->show();

  } else { // if( radioButton_FilterCreate->isChecked() )

    RenderWindowClosing( m_ActiveRenderWin );

    RenderWindow *renderWin = CreateNewRenderWindow(fileName);
    renderWin->show();
  }
}


void MainWindow::SetLighting(bool bLighting) {
  RenderWindow* w = GetActiveRenderWindow();
  if (w != NULL) {
    w->GetRenderer()->SetUseLigthing(bLighting);
  }
}

void MainWindow::SetSampleRate(int iValue) {
  if (m_ActiveRenderWin != NULL) m_ActiveRenderWin->GetRenderer()->SetSampleRateModifier(iValue/100.0f); 
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
  int iMaxSize = int(m_ActiveRenderWin->GetRenderer()->Get1DTrans()->GetSize());
  if (m_ActiveRenderWin != NULL) m_ActiveRenderWin->GetRenderer()->SetIsoValue(float(iValue)/float(iMaxSize));
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


void MainWindow::SetToggleGlobalBBoxLabel(bool bRenderBBox)
{
/// \todo
}

void MainWindow::SetToggleLocalBBoxLabel(bool bRenderBBox)
{
/// \todo
}

void MainWindow::ToggleGlobalBBox(bool bRenderBBox)
{
/// \todo
}

void MainWindow::ToggleLocalBBox(bool bRenderBBox)
{
/// \todo
}
