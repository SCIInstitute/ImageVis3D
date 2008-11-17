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


//!    File   : ImageVis3D_WindowHandling.cpp
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

using namespace std;


// ******************************************
// Geometry
// ******************************************

bool MainWindow::LoadGeometry() {
  QString fileName =
    QFileDialog::getOpenFileName(this, "Load Geometry",
				 ".",
				 "Geometry Files (*.geo)");
  if (!fileName.isEmpty()) {
    return LoadGeometry(fileName);
  } else return false;
}

bool MainWindow::SaveGeometry() {
  QString fileName = QFileDialog::getSaveFileName(this,
						  "Save Current Geometry",
						  ".",
						  "Geometry Files (*.geo)");
  if (!fileName.isEmpty()) {
    return SaveGeometry(fileName);
  } return false;
}

bool MainWindow::LoadGeometry(QString strFilename,
			      bool bSilentFail,
			      bool bRetryResource) {

  QSettings settings( strFilename, QSettings::IniFormat );

  settings.beginGroup("Geometry");
  bool bOK =
    restoreGeometry( settings.value("MainWinGeometry").toByteArray() ); 
  settings.endGroup();

  if (!bOK && bRetryResource) {
    string stdString(strFilename.toAscii());
    if (LoadGeometry(SysTools::GetFromResourceOnMac(stdString).c_str(),
		     true, false)) {
      return true;
    }
  }

  if (!bSilentFail && !bOK) {
    QString msg = tr("Error reading geometry file %1").arg(strFilename);
    QMessageBox::warning(this, tr("Error"), msg);
    return false;
  }

  return bOK;
}

bool MainWindow::SaveGeometry(QString strFilename) {
  QSettings settings( strFilename, QSettings::IniFormat ); 

  if (!settings.isWritable()) {
    QString msg = tr("Error saving geometry file %1").arg(strFilename);
    QMessageBox::warning(this, tr("Error"), msg);
    return false;
  }

  settings.beginGroup("Geometry");
  settings.setValue("MainWinGeometry", this->saveGeometry() ); 
  settings.endGroup();

  return true;
}

// ******************************************
// UI
// ******************************************

void MainWindow::setupUi(QMainWindow *MainWindow) {

  Ui_MainWindow::setupUi(MainWindow);

  QString qstrTitle = tr("%1 Version: %2").arg(windowTitle()).arg(IV3D_VERSION);
  setWindowTitle(qstrTitle);

  m_1DTransferFunction =
    new Q1DTransferFunction(m_MasterController, frame_1DTrans);
  verticalLayout_1DTrans->addWidget(m_1DTransferFunction);

  m_2DTransferFunction =
    new Q2DTransferFunction(m_MasterController, frame_2DTrans);
  verticalLayout_2DTrans->addWidget(m_2DTransferFunction);

  Use2DTrans();

  connect(verticalSlider_2DTransHistScale, SIGNAL(valueChanged(int)),
	  m_2DTransferFunction, SLOT(SetHistogtramScale(int)));
  connect(verticalSlider_1DTransHistScale, SIGNAL(valueChanged(int)),
	  m_1DTransferFunction, SLOT(SetHistogtramScale(int)));

  connect(m_2DTransferFunction, SIGNAL(SwatchChange()),
	  this, SLOT(Transfer2DSwatchesChanged()));
  connect(listWidget_Swatches, SIGNAL(currentRowChanged(int)),
	  m_2DTransferFunction, SLOT(Transfer2DSetActiveSwatch(int)));
  connect(listWidget_Swatches, SIGNAL(currentRowChanged(int)),
	  this, SLOT(Transfer2DUpdateSwatchButtons()));
  connect(listWidget_Gradient, SIGNAL(currentRowChanged(int)),
	  this, SLOT(Transfer2DUpdateGradientButtons()));  

  connect(pushButton_AddPoly,  SIGNAL(clicked()),
	  m_2DTransferFunction, SLOT(Transfer2DAddSwatch()));
  connect(pushButton_AddCircle,SIGNAL(clicked()),
	  m_2DTransferFunction, SLOT(Transfer2DAddCircleSwatch()));
  connect(pushButton_DelPoly,  SIGNAL(clicked()),
	  m_2DTransferFunction, SLOT(Transfer2DDeleteSwatch()));
  connect(pushButton_UpPoly,   SIGNAL(clicked()),
	  m_2DTransferFunction, SLOT(Transfer2DUpSwatch()));
  connect(pushButton_DownPoly, SIGNAL(clicked()),
	  m_2DTransferFunction, SLOT(Transfer2DDownSwatch()));

  for (unsigned int i = 0; i < ms_iMaxRecentFiles; ++i) {
    m_recentFileActs[i] = new QAction(this);
    m_recentFileActs[i]->setVisible(false);
    connect(m_recentFileActs[i], SIGNAL(triggered()),
	    this, SLOT(OpenRecentFile()));
    menuLast_Used_Projects->addAction(m_recentFileActs[i]);
  }

  // this widget is used to share the contexts amongst the render windows
  m_glShareWidget = new QGLWidget(this);
  this->horizontalLayout->addWidget(m_glShareWidget);

  DisableAllTrans();

  m_DebugOut = new QTOut(listWidget_3);
  m_MasterController.SetDebugOut(m_DebugOut);
  GetDebugViewMask();

  frame_Expand2DWidgets->hide();
}

// ******************************************
// Workspace
// ******************************************

void MainWindow::SetupWorkspaceMenu() {
  menu_Workspace->addAction(dockWidget_Tools->toggleViewAction());
  menu_Workspace->addAction(dockWidget_Filters->toggleViewAction());
  menu_Workspace->addSeparator();
  menu_Workspace->addAction(dockWidget_History->toggleViewAction());
  menu_Workspace->addAction(dockWidget_Information->toggleViewAction());
  menu_Workspace->addAction(dockWidget_Recorder->toggleViewAction());
  menu_Workspace->addAction(dockWidget_LockOptions->toggleViewAction());
  menu_Workspace->addAction(dockWidget_RenderOptions->toggleViewAction());
  menu_Workspace->addAction(dockWidget_ProgressView->toggleViewAction());
  menu_Workspace->addSeparator();
  menu_Workspace->addAction(dockWidget_1DTrans->toggleViewAction());
  menu_Workspace->addAction(dockWidget_2DTrans->toggleViewAction());
  menu_Workspace->addAction(dockWidget_IsoSurface->toggleViewAction());

  menu_Help->addAction(dockWidget_Debug->toggleViewAction());
}

void MainWindow::InitAllWorkspaces() {
  dockWidget_Tools->setVisible(false);
  dockWidget_Filters->setVisible(false);
  dockWidget_History->setVisible(false);
  dockWidget_Information->setVisible(false);
  dockWidget_Recorder->setVisible(false);
  dockWidget_LockOptions->setVisible(false);
  dockWidget_RenderOptions->setVisible(false);
  dockWidget_ProgressView->setVisible(false);
  dockWidget_1DTrans->setVisible(false);
  dockWidget_2DTrans->setVisible(false);
  dockWidget_IsoSurface->setVisible(false);
  dockWidget_Debug->setVisible(false);

  dockWidget_Tools->setFloating(true);
  dockWidget_Filters->setFloating(true);
  dockWidget_History->setFloating(true);
  dockWidget_Information->setFloating(true);
  dockWidget_Recorder->setFloating(true);
  dockWidget_LockOptions->setFloating(true);
  dockWidget_RenderOptions->setFloating(true);
  dockWidget_ProgressView->setFloating(true);
  dockWidget_1DTrans->setFloating(true);
  dockWidget_2DTrans->setFloating(true);
  dockWidget_IsoSurface->setFloating(true);
  dockWidget_Debug->setFloating(true);
}


bool MainWindow::LoadWorkspace() {
  QString fileName = QFileDialog::getOpenFileName(this,
						  "Load Workspace",
						  ".",
						  "Workspace Files (*.wsp)");
  if (!fileName.isEmpty()) {
    return LoadWorkspace(fileName);
  } else return false;
}

bool MainWindow::SaveWorkspace() {
  QString fileName = QFileDialog::getSaveFileName(this,
						  "Save Current Workspace",
						  ".",
						  "Workspace Files (*.wsp)");
  if (!fileName.isEmpty()) {
    return SaveWorkspace(fileName);
  } else return false;
}


bool MainWindow::LoadWorkspace(QString strFilename,
			       bool bSilentFail,
			       bool bRetryResource) {

  QSettings settings( strFilename, QSettings::IniFormat ); 

  settings.beginGroup("Geometry");
  bool bOK = restoreState( settings.value("DockGeometry").toByteArray() ); 
  settings.endGroup();

  if (!bOK && bRetryResource) {
    string stdString(strFilename.toAscii());

    if (LoadWorkspace(SysTools::GetFromResourceOnMac(stdString).c_str(),
		      true, false)) {
      m_strCurrentWorkspaceFilename =
	SysTools::GetFromResourceOnMac(stdString).c_str();
      return true;
    }
  }

  if (!bSilentFail && !bOK) {
    QString msg = tr("Error reading workspace file %1").arg(strFilename);
    QMessageBox::warning(this, tr("Error"), msg);
    return false;
  }

  m_strCurrentWorkspaceFilename = strFilename;

  return bOK;
}


bool MainWindow::SaveWorkspace(QString strFilename) {
  QSettings settings( strFilename, QSettings::IniFormat ); 

  if (!settings.isWritable()) {
    QString msg = tr("Error saving workspace file %1").arg(strFilename);
    QMessageBox::warning(this, tr("Error"), msg);
    return false;
  }

  settings.beginGroup("Geometry");
  settings.setValue("DockGeometry", this->saveState() ); 
  settings.endGroup();   

  return true;
}


bool MainWindow::ApplyWorkspace() {
  if (!m_strCurrentWorkspaceFilename.isEmpty())
    return LoadWorkspace(m_strCurrentWorkspaceFilename);
  else 
    return false;
}

// ******************************************
// Render Windows
// ******************************************

void MainWindow::CloneCurrentView() {
  RenderWindow *renderWin = CreateNewRenderWindow(GetActiveRenderWindow()->GetDatasetName());
  renderWin->show();
}

bool MainWindow::CheckRenderwindowFitness(RenderWindow *renderWin, bool bIfNotOkShowMessageAndCloseWindow) {
  if (renderWin) {
    bool bIsOK = renderWin->IsRenderSubsysOK();

    if (bIfNotOkShowMessageAndCloseWindow && !bIsOK) {
      m_MasterController.DebugOut()->Error("IOManager::CheckRenderwindowFitness","Unable to initialize the render window, see previous error messages for details.");

      // find window in mdi area
      for (int i = 0;i<mdiArea->subWindowList().size();i++) {
        QWidget* w = mdiArea->subWindowList().at(i)->widget();
        RenderWindow* subwindow = qobject_cast<RenderWindow*>(w);

        if (subwindow == renderWin)  {
          mdiArea->setActiveSubWindow(mdiArea->subWindowList().at(i));
          mdiArea->closeActiveSubWindow();  // we assume that CheckRenderwindowFitness is always called on the active subwindow
          break;
        }
      }
	    QMessageBox::critical(this, "Error during render window initialization.", "The system was unable to open a render window, please check the error log for details (Menu -> \"Help\" -> \"Debug Window\").");
    }
    return bIsOK;
  } return false;
}

/// \todo ARS Need to be able to CreateNewRenderWindow based on memory only

RenderWindow* MainWindow::CreateNewRenderWindow(QString dataset)
{
  static unsigned int iCounter = 0;

  RenderWindow *renderWin =
    new RenderWindow(m_MasterController, m_eVolumeRendererType, dataset,
		     iCounter++, m_glShareWidget, this);
  renderWin->SetColors(m_vBackgroundColors, m_vTextColor);
  renderWin->SetBlendPrecision(AbstrRenderer::EBlendPrecision(m_iBlendPrecisionMode));
  renderWin->SetPerfMeasures(m_iMinFramerate, m_iLODDelay/10, m_iActiveTS, m_iInactiveTS);

  mdiArea->addSubWindow(renderWin);
  listWidget_Lock->addItem(renderWin->GetWindowID());

  connect(renderWin, SIGNAL(WindowActive(RenderWindow*)),
	  this, SLOT(RenderWindowActive(RenderWindow*)));
  connect(renderWin, SIGNAL(WindowClosing(RenderWindow*)),
	  this, SLOT(RenderWindowClosing(RenderWindow*)));

  return renderWin;
}


void MainWindow::RenderWindowActive(RenderWindow* sender) {

  if (m_ActiveRenderWin != sender) {
    m_MasterController.DebugOut()->
      Message("MainWindow::RenderWindowActive",
	      "ACK that %s is now active",
	      sender->GetDatasetName().toStdString().c_str());
    m_ActiveRenderWin = sender;

    if (!CheckRenderwindowFitness(m_ActiveRenderWin)) return;

    m_1DTransferFunction->
      SetData(sender->GetRenderer()->GetDataSet()->Get1DHistogram(),
	      sender->GetRenderer()->Get1DTrans());
    m_1DTransferFunction->update();
    m_2DTransferFunction->
      SetData(sender->GetRenderer()->GetDataSet()->Get2DHistogram(),
	      sender->GetRenderer()->Get2DTrans());
    m_2DTransferFunction->update();

    AbstrRenderer::ERenderMode e = m_ActiveRenderWin->GetRenderer()->GetRendermode();

    switch (e) {
      case AbstrRenderer::RM_1DTRANS    : Use1DTrans(); break;
      case AbstrRenderer::RM_2DTRANS    : Use2DTrans(); break;
      case AbstrRenderer::RM_ISOSURFACE : UseIso(); break;
      default : m_MasterController.DebugOut()->
		                Error("MainWindow::RenderWindowActive",
		                      "unknown rendermode from %s",
		                      sender->GetDatasetName().toStdString().c_str());
                break;
    }

    checkBox_Lighting->setChecked(m_ActiveRenderWin->GetRenderer()->GetUseLigthing());
    SetSampleRateSlider(int(m_ActiveRenderWin->GetRenderer()->GetSampleRateModifier()*100));
    int iRange = int(m_ActiveRenderWin->GetRenderer()->Get1DTrans()->GetSize());
    SetIsoValueSlider(int(m_ActiveRenderWin->GetRenderer()->GetIsoValue()*iRange), iRange);

    DOUBLEVECTOR3 vfRescaleFactors =  m_ActiveRenderWin->GetRenderer()->GetRescaleFactors();
    doubleSpinBox_RescaleX->setValue(vfRescaleFactors.x);
    doubleSpinBox_RescaleY->setValue(vfRescaleFactors.y);
    doubleSpinBox_RescaleZ->setValue(vfRescaleFactors.z);

    SetToggleGlobalBBoxLabel(m_ActiveRenderWin->GetRenderer()->GetGlobalBBox());
    SetToggleLocalBBoxLabel(m_ActiveRenderWin->GetRenderer()->GetLocalBBox());
    ClearProgressView();
  }
}

void MainWindow::SetRescaleFactors() {
  DOUBLEVECTOR3 vfRescaleFactors;
  vfRescaleFactors.x = doubleSpinBox_RescaleX->value();
  vfRescaleFactors.y = doubleSpinBox_RescaleY->value();
  vfRescaleFactors.z = doubleSpinBox_RescaleZ->value();
  m_ActiveRenderWin->GetRenderer()->SetRescaleFactors(vfRescaleFactors);
}


void MainWindow::RenderWindowClosing(RenderWindow* sender) {
  m_MasterController.DebugOut()->
    Message("MainWindow::RenderWindowClosing",
	    "ACK that %s is now closing",
	    sender->GetDatasetName().toStdString().c_str());

  QList<QListWidgetItem*> l =
    listWidget_Lock->findItems(sender->GetWindowID(),  Qt::MatchExactly);
  assert(l.size() == 1); // if l.size() != 1 something went wrong
			                   // during the creation of the list
  delete l[0];

  m_ActiveRenderWin = NULL;
  disconnect(sender, SIGNAL(WindowActive(RenderWindow*)),
	     this, SLOT(RenderWindowActive(RenderWindow*)));
  disconnect(sender, SIGNAL(WindowClosing(RenderWindow*)),
	     this, SLOT(RenderWindowClosing(RenderWindow*)));

  m_1DTransferFunction->SetData(NULL, NULL);
  m_1DTransferFunction->update();
  m_2DTransferFunction->SetData(NULL, NULL);
  m_2DTransferFunction->update();

  DisableAllTrans();
  ClearProgressView();
}



void MainWindow::ToggleRenderWindowView2x2() {
  RenderWindow* win = GetActiveRenderWindow();
  if (win) win->ToggleRenderWindowView2x2();
}


void MainWindow::ToggleRenderWindowViewSingle() {
  RenderWindow* win = GetActiveRenderWindow();
  if (win) win->ToggleRenderWindowViewSingle();
}


RenderWindow* MainWindow::GetActiveRenderWindow()
{
  if (QMdiSubWindow* activeSubWindow = mdiArea->activeSubWindow())
    return qobject_cast<RenderWindow*>(activeSubWindow->widget());
  else
    return NULL;
}


void MainWindow::CheckForRedraw() {
  for (int i = 0;i<mdiArea->subWindowList().size();i++) {
    QWidget* w = mdiArea->subWindowList().at(i)->widget();
    qobject_cast<RenderWindow*>(w)->CheckForRedraw();
  }
}

// ******************************************
// Menus
// ******************************************

void MainWindow::OpenRecentFile(){

  QAction *action = qobject_cast<QAction *>(sender());
  if (action) LoadDataset(action->data().toString());
}

void MainWindow::UpdateMenus() {
  bool bHasMdiChild = (GetActiveRenderWindow() != 0);
  actionSave_Dataset->setEnabled(bHasMdiChild);

  actionGo_Fullscreen->setEnabled(bHasMdiChild);
  actionCascade->setEnabled(bHasMdiChild);
  actionTile->setEnabled(bHasMdiChild);
  actionNext->setEnabled(bHasMdiChild);
  actionPrevious->setEnabled(bHasMdiChild);
  action2_x_2_View->setEnabled(bHasMdiChild);
  actionSinge_View->setEnabled(bHasMdiChild);
  actionCloneCurrentView->setEnabled(bHasMdiChild);

  actionBox->setEnabled(bHasMdiChild);
  actionPoly_Line->setEnabled(bHasMdiChild);
  actionSelect_All->setEnabled(bHasMdiChild);
  actionDelete_Selection->setEnabled(bHasMdiChild);
  actionInvert_Selection->setEnabled(bHasMdiChild);
  actionStastistcs->setEnabled(bHasMdiChild);
  actionUndo->setEnabled(bHasMdiChild);
  actionRedo->setEnabled(bHasMdiChild);  
}

// ******************************************
// Recent Files
// ******************************************

void MainWindow::ClearMRUList()
{
  QSettings settings;
  QStringList files;
  files.clear();
  settings.setValue("Menu/MRU", files);

  UpdateMRUActions();
}


void MainWindow::AddFileToMRUList(const QString &fileName)
{
  QSettings settings;
  QStringList files = settings.value("Menu/MRU").toStringList();

  files.removeAll(fileName);
  files.prepend(fileName);
  while ((unsigned int)(files.size()) > ms_iMaxRecentFiles)
    files.removeLast();

  settings.setValue("Menu/MRU", files);

  UpdateMRUActions();
}


QString MainWindow::strippedName(const QString &fullFileName)
{
  return QFileInfo(fullFileName).fileName();
}

void MainWindow::UpdateMRUActions()
{
  QSettings settings;
  QStringList files = settings.value("menu/MRU").toStringList();

  int numRecentFiles = qMin(files.size(), (int)ms_iMaxRecentFiles);

  for (int i = 0; i < numRecentFiles; ++i) {
    QString text = tr("&%1 %2").arg(i + 1).arg(strippedName(files[i]));
    m_recentFileActs[i]->setText(text);
    m_recentFileActs[i]->setData(files[i]);
    m_recentFileActs[i]->setVisible(true);
  }

  for (unsigned int j = numRecentFiles; j < ms_iMaxRecentFiles; ++j)
    m_recentFileActs[j]->setVisible(false);
}


void MainWindow::Collapse2DWidgets() {
  frame_2DTransEditWrapper->hide();
  frame_Expand2DWidgets->show();
}

void MainWindow::Expand2DWidgets() {
  frame_2DTransEditWrapper->show();
  frame_Expand2DWidgets->hide();
}


void MainWindow::ShowAbout()
{
  QMessageBox::about(this, "ImageVis3D "IV3D_VERSION,
    tr("This is <b>ImageVis3D</b> "IV3D_VERSION". Copyrigth 2008 by the Scientific Computing and Imaging (SCI) Institute. This version is for internal testing only. Please report bugs to jens@sci.utah.edu"));
}