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
#include "RenderWindowGL.h"
#include "RenderWindowDX.h"

#include <QtCore/QTimer>
#include <QtGui/QMdiSubWindow>
#include <QtGui/QFileDialog>
#include <QtCore/QSettings>
#include <QtGui/QMessageBox>
#include <QtGui/QInputDialog>
#include <QtGui/QColorDialog>
#include <QtNetwork/QHttp>

#include <fstream>
#include <iostream>
#include <string>
#include "../Tuvok/Basics/SysTools.h"

using namespace std;


// ******************************************
// Geometry
// ******************************************

bool MainWindow::LoadGeometry() {
  QSettings settings;
  QString strLastDir = settings.value("Folders/LoadGeometry", ".").toString();

  QFileDialog::Options options;
#ifdef TUVOK_OS_APPLE
  options |= QFileDialog::DontUseNativeDialog;
#endif
  QString selectedFilter;

  QString fileName =
    QFileDialog::getOpenFileName(this, "Load Geometry",
         strLastDir,
         "Geometry Files (*.geo)",&selectedFilter, options);
  if (!fileName.isEmpty()) {
    settings.setValue("Folders/LoadGeometry", QFileInfo(fileName).absoluteDir().path());
    return LoadGeometry(fileName);
  } else return false;
}

bool MainWindow::SaveGeometry() {
  QSettings settings;
  QString strLastDir = settings.value("Folders/SaveGeometry", ".").toString();

  QFileDialog::Options options;
#ifdef TUVOK_OS_APPLE
  options |= QFileDialog::DontUseNativeDialog;
#endif
  QString selectedFilter;

  QString fileName = QFileDialog::getSaveFileName(this,
              "Save Current Geometry",
              strLastDir,
              "Geometry Files (*.geo)",&selectedFilter, options);
  if (!fileName.isEmpty()) {
    fileName = SysTools::CheckExt(string(fileName.toAscii()), "geo").c_str();
    settings.setValue("Folders/SaveGeometry", QFileInfo(fileName).absoluteDir().path());
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
    ShowWarningDialog( tr("Error"), msg);
    return false;
  }

  return bOK;
}

bool MainWindow::SaveGeometry(QString strFilename) {
  QSettings settings( strFilename, QSettings::IniFormat ); 

  if (!settings.isWritable()) {
    QString msg = tr("Error saving geometry file %1").arg(strFilename);
    ShowWarningDialog( tr("Error"), msg);
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

void MainWindow::SetTitle() {
  QString qstrTitle;
  if (m_bShowVersionInTitle) 
    qstrTitle = tr("ImageVis3D Version: %1 %2 [Tuvok %3 %4 %5]").arg(IV3D_VERSION).arg(IV3D_VERSION_TYPE).arg(TUVOK_VERSION).arg(TUVOK_VERSION_TYPE).arg(TUVOK_DETAILS);
  else
    qstrTitle = tr("ImageVis3D");
  setWindowTitle(qstrTitle);
}


void MainWindow::setupUi(QMainWindow *MainWindow) {

  Ui_MainWindow::setupUi(MainWindow);

  SetTitle();

  m_1DTransferFunction =
    new Q1DTransferFunction(m_MasterController, frame_1DTrans);
  verticalLayout_1DTrans->addWidget(m_1DTransferFunction);

  m_2DTransferFunction =
    new Q2DTransferFunction(m_MasterController, frame_2DTrans);
  verticalLayout_2DTrans->addWidget(m_2DTransferFunction);

  connect(verticalSlider_2DTransHistScale, SIGNAL(valueChanged(int)),
    m_2DTransferFunction, SLOT(SetHistogtramScale(int)));
  connect(verticalSlider_1DTransHistScale, SIGNAL(valueChanged(int)),
    m_1DTransferFunction, SLOT(SetHistogtramScale(int)));

  verticalSlider_2DTransHistScale->setValue(1500);

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
  QGLFormat fmt;
  fmt.setAlpha(true);
  fmt.setRgba(true);
  m_glShareWidget = new QGLWidget(fmt,this);
  this->horizontalLayout->addWidget(m_glShareWidget);

  DisableAllTrans();

  m_DebugOut = new QTOut(listWidget_DebugOut);
  m_MasterController.SetDebugOut(m_DebugOut);
  GetDebugViewMask();

  frame_Expand2DWidgets->hide();
  UpdateLockView();

  QSettings settings;
  QString fileName = settings.value("Files/CaptureFilename", lineEditCaptureFile->text()).toString();
  lineEditCaptureFile->setText(fileName);
  checkBox_PreserveTransparency->setChecked(settings.value("PreserveTransparency", true).toBool());

  m_pHttp = new QHttp(this);
  connect(m_pHttp, SIGNAL(requestFinished(int, bool)), this, SLOT(httpRequestFinished(int, bool)));
  connect(m_pHttp, SIGNAL(responseHeaderReceived(const QHttpResponseHeader &)), this, SLOT(readResponseHeader(const QHttpResponseHeader &)));
}

// ******************************************
// Workspace
// ******************************************

void MainWindow::SetupWorkspaceMenu() {

/// \todo Implement more functionality of the other workspaces

//  menu_Workspace->addAction(dockWidget_Tools->toggleViewAction());
//  menu_Workspace->addAction(dockWidget_Filters->toggleViewAction());
//  menu_Workspace->addSeparator();
//  menu_Workspace->addAction(dockWidget_History->toggleViewAction());
//  menu_Workspace->addAction(dockWidget_Information->toggleViewAction());

  radioButton_ToolsLock->setVisible(false);
  radioButton_FiltersLock->setVisible(false);

  menu_Workspace->addAction(dockWidget_RenderOptions->toggleViewAction());
  menu_Workspace->addAction(dockWidget_ProgressView->toggleViewAction());
  menu_Workspace->addSeparator();
  menu_Workspace->addAction(dockWidget_1DTrans->toggleViewAction());
  menu_Workspace->addAction(dockWidget_2DTrans->toggleViewAction());
  menu_Workspace->addAction(dockWidget_IsoSurface->toggleViewAction());
  menu_Workspace->addSeparator();
  menu_Workspace->addAction(dockWidget_LockOptions->toggleViewAction());
  menu_Workspace->addAction(dockWidget_Recorder->toggleViewAction());
  menu_Workspace->addAction(dockWidget_Stereo->toggleViewAction());

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
  dockWidget_Stereo->setVisible(false);

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
  dockWidget_Stereo->setFloating(true);
}


bool MainWindow::LoadWorkspace() {
  QSettings settings;
  QString strLastDir = settings.value("Folders/LoadWorkspace", ".").toString();

  QFileDialog::Options options;
#ifdef TUVOK_OS_APPLE
  options |= QFileDialog::DontUseNativeDialog;
#endif
  QString selectedFilter;

  QString fileName = QFileDialog::getOpenFileName(this,
              "Load Workspace",
              strLastDir,
              "Workspace Files (*.wsp)",&selectedFilter, options);
  if (!fileName.isEmpty()) {
    settings.setValue("Folders/LoadWorkspace", QFileInfo(fileName).absoluteDir().path());
    return LoadWorkspace(fileName);
  } else return false;
}

bool MainWindow::SaveWorkspace() {
  QSettings settings;
  QString strLastDir = settings.value("Folders/SaveWorkspace", ".").toString();

  QFileDialog::Options options;
#ifdef TUVOK_OS_APPLE
  options |= QFileDialog::DontUseNativeDialog;
#endif
  QString selectedFilter;

  QString fileName = QFileDialog::getSaveFileName(this,
              "Save Current Workspace",
              strLastDir,
              "Workspace Files (*.wsp)",&selectedFilter, options);
  if (!fileName.isEmpty()) {
    fileName = SysTools::CheckExt(string(fileName.toAscii()), "wsp").c_str();
    settings.setValue("Folders/SaveWorkspace", QFileInfo(fileName).absoluteDir().path());
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
    ShowWarningDialog( tr("Error"), msg);
    return false;
  }

  m_strCurrentWorkspaceFilename = strFilename;

  return bOK;
}


bool MainWindow::SaveWorkspace(QString strFilename) {
  QSettings settings( strFilename, QSettings::IniFormat ); 

  if (!settings.isWritable()) {
    QString msg = tr("Error saving workspace file %1").arg(strFilename);
    ShowWarningDialog( tr("Error"), msg);
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


void MainWindow::ResizeCurrentView(int iSizeX, int iSizeY) {
  if (!m_pActiveRenderWin) return;
  mdiArea->activeSubWindow()->resize(iSizeX, iSizeY);
}

void MainWindow::CloseCurrentView() {
  if (!m_pActiveRenderWin) return;
  mdiArea->activeSubWindow()->close();
}

void MainWindow::CloneCurrentView() {
  if (!m_pActiveRenderWin) return;
  RenderWindow *renderWin = CreateNewRenderWindow(m_pActiveRenderWin->GetDatasetName());

  renderWin->CloneViewState(m_pActiveRenderWin);
  renderWin->CloneRendermode(m_pActiveRenderWin);

  if (m_bAutoLockClonedWindow) 
    for (size_t i = 0;i<RenderWindow::ms_iLockCount;i++) SetLock(i, renderWin, m_pActiveRenderWin); 

  QMdiSubWindow * pActiveWin = mdiArea->activeSubWindow(); // as "show" toggles the active renderwin we need to remeber it
  renderWin->GetQtWidget()->show();
  RenderWindowActive(renderWin);
  mdiArea->activeSubWindow()->resize(pActiveWin->size().width(), pActiveWin->size().height());
}

bool MainWindow::CheckRenderwindowFitness(RenderWindow *renderWin, bool bIfNotOkShowMessageAndCloseWindow) {
  if (renderWin) {
    bool bIsOK = renderWin->IsRenderSubsysOK();
    if (!bIsOK) 
      m_pActiveRenderWin = NULL;
    else
      m_MasterController.DebugOut()->Message("IOManager::CheckRenderwindowFitness","Renderwindow healthy.");

    if (bIfNotOkShowMessageAndCloseWindow && !bIsOK) {
      m_MasterController.DebugOut()->Error("IOManager::CheckRenderwindowFitness","Unable to initialize the render window, see previous error messages for details.");

      // find window in mdi area
      for (int i = 0;i<mdiArea->subWindowList().size();i++) {
        QWidget* w = mdiArea->subWindowList().at(i)->widget();
        RenderWindow* subwindow = WidgetToRenderWin(w);

        if (subwindow == renderWin)  {
          mdiArea->setActiveSubWindow(mdiArea->subWindowList().at(i));
          mdiArea->closeActiveSubWindow();
          break;
        }
      }
      ShowCriticalDialog( "Error during render window initialization.", "The system was unable to open a render window, please check the error log for details (Menu -> \"Help\" -> \"Debug Window\").");
    }
    return bIsOK;
  } return false;
}

RenderWindow* MainWindow::CreateNewRenderWindow(QString dataset)
{
  static unsigned int iCounter = 0;
  RenderWindow *renderWin;

  #if defined(_WIN32) && defined(USE_DIRECTX)
    if (m_eVolumeRendererType >= MasterController::DIRECTX_SBVR) {
      renderWin = new RenderWindowDX(m_MasterController, m_eVolumeRendererType, dataset, 
                                       iCounter++, m_bPowerOfTwo, m_bDownSampleTo8Bits, 
                                       m_bDisableBorder, this, 0);
    } else {
      QGLFormat fmt;
      fmt.setRgba(true);
      fmt.setAlpha(true);
      renderWin = new RenderWindowGL(m_MasterController, m_eVolumeRendererType, dataset, 
                                     iCounter++, m_bPowerOfTwo, m_bDownSampleTo8Bits, 
                                     m_bDisableBorder, m_glShareWidget, fmt, this, 0);
    }
  #else
    if (m_eVolumeRendererType >= MasterController::DIRECTX_SBVR) {
      ShowInformationDialog( "No DirectX Support", "The system was unable to open a DirectX 10 render window, falling back to OpenGL. Please check your settings.");
      m_MasterController.DebugOut()->Message("MainWindow::CreateNewRenderWindow","The system was unable to open a DirectX 10 render window, falling back to OpenGL. Please check your settings.");

      m_eVolumeRendererType = MasterController::EVolumeRendererType(int(m_eVolumeRendererType) - int(MasterController::DIRECTX_SBVR) );
    }
    QGLFormat fmt;
    fmt.setAlpha(true);
    fmt.setRgba(true);
    renderWin = new RenderWindowGL(m_MasterController, m_eVolumeRendererType, dataset, 
                                   iCounter++, m_bPowerOfTwo, m_bDownSampleTo8Bits, 
                                   m_bDisableBorder, m_glShareWidget, fmt, this, 0);
  #endif

  if (renderWin && renderWin->GetRenderer()) {
    ApplySettings(renderWin);
  }

  mdiArea->addSubWindow(renderWin->GetQtWidget());
  connect(renderWin->GetQtWidget(), SIGNAL(WindowActive(RenderWindow*)), this, SLOT(RenderWindowActive(RenderWindow*)));
  connect(renderWin->GetQtWidget(), SIGNAL(WindowClosing(RenderWindow*)), this, SLOT(RenderWindowClosing(RenderWindow*)));
  connect(renderWin->GetQtWidget(), SIGNAL(RenderWindowViewChanged(int)), this, SLOT(RenderWindowViewChanged(int)));
  connect(renderWin->GetQtWidget(), SIGNAL(StereoDisabled()), this, SLOT(StereoDisabled()));

  if(m_pActiveRenderWin != renderWin) {
    QCoreApplication::processEvents();
#ifdef TUVOK_OS_APPLE
    // HACK: For some reason on the Mac we need to set the active sub window,
    // re-process events, and then call our activation function ... doesn't
    // seem to happen automagically.
    QList<QMdiSubWindow *>::iterator iter;
    for(iter = mdiArea->subWindowList().begin(); iter !=
      mdiArea->subWindowList().end();  ++iter) {
      if(renderWin->GetQtWidget() == (*iter)->widget()) {
        mdiArea->setActiveSubWindow(*iter);
      }
    }
    QCoreApplication::processEvents();
    RenderWindowActive(renderWin);
#endif
  }
  
  return renderWin;
}


void MainWindow::RenderWindowActive(RenderWindow* sender) {
  if (m_pActiveRenderWin != sender) {
    m_MasterController.DebugOut()->
      Message("MainWindow::RenderWindowActive",
        "ACK that %s is now active",
        sender->GetDatasetName().toStdString().c_str());
    m_pActiveRenderWin = sender;

    if (!CheckRenderwindowFitness(m_pActiveRenderWin)) {
      return;
    }

    m_1DTransferFunction->
      SetData(sender->GetRenderer()->GetDataSet()->Get1DHistogram(),
        sender->GetRenderer()->Get1DTrans());
    m_1DTransferFunction->update();
    m_2DTransferFunction->
      SetData(sender->GetRenderer()->GetDataSet()->Get2DHistogram(),
        sender->GetRenderer()->Get2DTrans());
    m_2DTransferFunction->update();

    AbstrRenderer::ERenderMode e = m_pActiveRenderWin->GetRendermode();

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

    dockWidget_Stereo->setEnabled(true);
    checkBox_Stereo->setChecked(m_pActiveRenderWin->GetRenderer()->GetStereo());
    horizontalSlider_EyeDistance->setValue(int(m_pActiveRenderWin->GetRenderer()->GetStereoEyeDist()*100));
    horizontalSlider_FocalLength->setValue(int(m_pActiveRenderWin->GetRenderer()->GetStereoFocalLength()*10));

    checkBox_Lighting->setChecked(m_pActiveRenderWin->GetRenderer()->GetUseLighting());
    SetSampleRateSlider(int(m_pActiveRenderWin->GetRenderer()->GetSampleRateModifier()*100));
    int iRange = int(m_pActiveRenderWin->GetRenderer()->Get1DTrans()->GetSize());
    SetIsoValueSlider(int(m_pActiveRenderWin->GetRenderer()->GetIsoValue()*iRange), iRange);

    DOUBLEVECTOR3 vfRescaleFactors =  m_pActiveRenderWin->GetRenderer()->GetRescaleFactors();
    doubleSpinBox_RescaleX->setValue(vfRescaleFactors.x);
    doubleSpinBox_RescaleY->setValue(vfRescaleFactors.y);
    doubleSpinBox_RescaleZ->setValue(vfRescaleFactors.z);

    SetToggleGlobalBBoxLabel(m_pActiveRenderWin->GetRenderer()->GetGlobalBBox());
    SetToggleLocalBBoxLabel(m_pActiveRenderWin->GetRenderer()->GetLocalBBox());
    ClearProgressView();

    ToggleClearViewControls(iRange);
    UpdateLockView();
  }
}

void MainWindow::ToggleClearViewControls(int iRange) {
  if (m_pActiveRenderWin->GetRenderer()->SupportsClearView()) {
    checkBox_ClearView->setVisible(true);
    frame_ClearView->setVisible(true);

    checkBox_ClearView->setChecked(m_pActiveRenderWin->GetRenderer()->GetCV());
  } else {
    checkBox_ClearView->setVisible(false);
    frame_ClearView->setVisible(false);
  }

  SetFocusIsoValueSlider(int(m_pActiveRenderWin->GetRenderer()->GetCVIsoValue()*iRange), iRange);
  SetFocusSizeValueSlider(99-int(m_pActiveRenderWin->GetRenderer()->GetCVSize()*9.9f));
  SetContextScaleValueSlider(int(m_pActiveRenderWin->GetRenderer()->GetCVContextScale()*10.0f));
  SetBorderSizeValueSlider(int(99-m_pActiveRenderWin->GetRenderer()->GetCVBorderScale()));
}

void MainWindow::SetRescaleFactors() {
  if (!m_pActiveRenderWin) return;
  DOUBLEVECTOR3 vfRescaleFactors;
  vfRescaleFactors.x = doubleSpinBox_RescaleX->value();
  vfRescaleFactors.y = doubleSpinBox_RescaleY->value();
  vfRescaleFactors.z = doubleSpinBox_RescaleZ->value();
  m_pActiveRenderWin->GetRenderer()->SetRescaleFactors(vfRescaleFactors);
}


void MainWindow::StereoDisabled() {
  checkBox_Stereo->setChecked(false);
}

void MainWindow::RenderWindowViewChanged(int iMode) {
  groupBox_MovieCapture->setEnabled(iMode == 0);
}

void MainWindow::RenderWindowClosing(RenderWindow* sender) {
  m_MasterController.DebugOut()->
    Message("MainWindow::RenderWindowClosing",
      "ACK that %s is now closing",
      sender->GetDatasetName().toStdString().c_str());

  RemoveAllLocks(sender);

  m_pActiveRenderWin = NULL;
  disconnect(sender->GetQtWidget(), SIGNAL(WindowActive(RenderWindow*)),  this, SLOT(RenderWindowActive(RenderWindow*)));
  disconnect(sender->GetQtWidget(), SIGNAL(WindowClosing(RenderWindow*)), this, SLOT(RenderWindowClosing(RenderWindow*)));
  disconnect(sender->GetQtWidget(), SIGNAL(RenderWindowViewChanged(int)), this, SLOT(RenderWindowViewChanged(int)));
  disconnect(sender->GetQtWidget(), SIGNAL(StereoDisabled()), this, SLOT(StereoDisabled()));

  m_1DTransferFunction->SetData(NULL, NULL);
  m_1DTransferFunction->update();
  m_2DTransferFunction->SetData(NULL, NULL);
  m_2DTransferFunction->update();

  DisableAllTrans();

  dockWidget_Stereo->setEnabled(false);

  ClearProgressView();

  UpdateLockView();
}



void MainWindow::ToggleRenderWindowView2x2() {
  if (m_pActiveRenderWin) m_pActiveRenderWin->ToggleRenderWindowView2x2();
}


void MainWindow::ToggleRenderWindowViewSingle() {
  if (m_pActiveRenderWin) m_pActiveRenderWin->ToggleRenderWindowViewSingle();
}

void MainWindow::CheckForRedraw() {
  for (int i = 0;i<mdiArea->subWindowList().size();i++) {
    QWidget* w = mdiArea->subWindowList().at(i)->widget();
    RenderWindow* r = WidgetToRenderWin(w);
    r->CheckForRedraw();
  }
}

// ******************************************
// Menus
// ******************************************

void MainWindow::OpenRecentFile(){
  QAction *action = qobject_cast<QAction *>(sender());

  if (SysTools::FileExists(string(action->data().toString().toAscii()))) {
    if (action) LoadDataset(action->data().toString());
  } else {
    QString strText = tr("File %1 not found.").arg(action->data().toString());
    m_MasterController.DebugOut()->Error("MainWindow::OpenRecentFile", strText.toStdString().c_str());
    strText = strText + " Do you want to remove the file from the MRU list?";
    if (QMessageBox::Yes == QMessageBox::question(this, "Load Error", strText, QMessageBox::Yes, QMessageBox::No)) {

      int iIndex = -1;
      for (int i = 0; i < int(ms_iMaxRecentFiles); ++i) {
        if (m_recentFileActs[i] == action) {
          iIndex = i;
          break;
        }
      }

      if (iIndex > -1) {
        QSettings settings;
        QStringList files = settings.value("Menu/MRU").toStringList();
        files.removeAt(iIndex);
        settings.setValue("Menu/MRU", files);
        UpdateMRUActions();
      }
    }
    return;
  }
}

void MainWindow::UpdateMenus() {
  bool bHasMdiChild = mdiArea->subWindowList().size() > 0;
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

  /// \todo implement all of the features we are hiding here
  actionBox->setVisible(false);
  actionPoly_Line->setVisible(false);
  actionSelect_All->setVisible(false);
  actionDelete_Selection->setVisible(false);
  actionInvert_Selection->setVisible(false);
  actionStastistcs->setVisible(false);
  actionUndo->setVisible(false);
  actionRedo->setVisible(false); 
  actionSave_Dataset->setVisible(false); 

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


void MainWindow::UpdateMRUActions()
{
  QSettings settings;
  QStringList files = settings.value("Menu/MRU").toStringList();

  int numRecentFiles = qMin(files.size(), (int)ms_iMaxRecentFiles);

  for (int i = 0; i < numRecentFiles; ++i) {
    QString text = tr("&%1 %2").arg(i + 1).arg(QFileInfo(files[i]).fileName());
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

void MainWindow::Show1DTrans() {
  dockWidget_1DTrans->setVisible(true);
}

void MainWindow::Show2DTrans() {
  dockWidget_2DTrans->setVisible(true);
}

void MainWindow::ShowIsoEdit() {
  dockWidget_IsoSurface->setVisible(true);
}

void MainWindow::ShowCriticalDialog(QString strTitle, QString strMessage) {
  if (!m_bScriptMode) QMessageBox::critical(this, strTitle, strMessage);
}

void MainWindow::ShowInformationDialog(QString strTitle, QString strMessage) {
  if (!m_bScriptMode) QMessageBox::information(this, strTitle, strMessage);
}

void MainWindow::ShowWarningDialog(QString strTitle, QString strMessage) {
  if (!m_bScriptMode) QMessageBox::warning(this, strTitle, strMessage);
}
