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
#include "../Tuvok/Controller/Controller.h"
#include "../Tuvok/Basics/SysTools.h"

using namespace std;


// ******************************************
// Geometry
// ******************************************

bool MainWindow::LoadGeometry() {
  QSettings settings;
  QString strLastDir = settings.value("Folders/LoadGeometry", ".").toString();

  QFileDialog::Options options;
#ifdef DETECTED_OS_APPLE
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
#ifdef DETECTED_OS_APPLE
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

bool MainWindow::LoadDefaultGeometry() {
  QSettings settings;
  if (settings.contains("Geometry/MainWinGeometry"))
    return restoreGeometry( settings.value("Geometry/MainWinGeometry").toByteArray() );
  else 
    return false;
}

void MainWindow::SaveDefaultGeometry() {
  QSettings settings;
  settings.setValue("Geometry/MainWinGeometry", saveGeometry() );
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

  m_pQLightPreview =
    new QLightPreview(frame_lightPreview);
  horizontalLayout_lightPreview->addWidget(m_pQLightPreview);


  connect(verticalSlider_2DTransHistScale, SIGNAL(valueChanged(int)),
    m_2DTransferFunction, SLOT(SetHistogramScale(int)));
  connect(verticalSlider_1DTransHistScale, SIGNAL(valueChanged(int)),
    m_1DTransferFunction, SLOT(SetHistogramScale(int)));

  // These values need to be different than the initial values set via Qt's
  // `designer'.  It ensures that setValue generates a `change' event, which in
  // turn makes sure the initial rendering of the TF histograms match what the
  // value on the slider is.
  verticalSlider_2DTransHistScale->setValue(1500);
  verticalSlider_1DTransHistScale->setValue(500);

  connect(m_2DTransferFunction, SIGNAL(SwatchChange()),
    this, SLOT(Transfer2DSwatchesChanged()));
  connect(m_2DTransferFunction, SIGNAL(SwatchTypeChange(int)),
    this, SLOT(Transfer2DSwatcheTypeChanged(int)));
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

  connect(pushButton_NewTriangle,SIGNAL(clicked()),
    m_2DTransferFunction, SLOT(Transfer2DAddPseudoTrisSwatch()));
  connect(pushButton_NewRectangle,SIGNAL(clicked()),
    m_2DTransferFunction, SLOT(Transfer2DAddRectangleSwatch()));
  connect(pushButton_DelPoly_SimpleUI,  SIGNAL(clicked()),
    m_2DTransferFunction, SLOT(Transfer2DDeleteSwatch()));

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

    m_recentWSFileActs[i] = new QAction(this);
    m_recentWSFileActs[i]->setVisible(false);
    connect(m_recentWSFileActs[i], SIGNAL(triggered()),
      this, SLOT(OpenRecentWSFile()));
    menuMost_Recently_Used_Workspaces->addAction(m_recentWSFileActs[i]);
  }


  setWindowIcon(QIcon(QPixmap::fromImage(QImage(":/Resources/icon_16.png"))));

  // this widget is used to share the contexts amongst the render windows
  QGLFormat fmt;
  fmt.setAlpha(true);
  fmt.setRgba(true);
  m_glShareWidget = new QGLWidget(fmt,this);
  this->horizontalLayout->addWidget(m_glShareWidget);

  DisableAllTrans();

  m_pDebugOut = new QTOut(listWidget_DebugOut);
  m_MasterController.AddDebugOut(m_pDebugOut);
  GetDebugViewMask();

  frame_Expand2DWidgets->hide();
  frame_Simple2DTransControls->hide();
  UpdateLockView();

  QSettings settings;
  QString fileName = settings.value("Files/CaptureFilename", lineEditCaptureFile->text()).toString();
  lineEditCaptureFile->setText(fileName);
  checkBox_PreserveTransparency->setChecked(settings.value("PreserveTransparency", true).toBool());

  m_pHttp = new QHttp(this);
  connect(m_pHttp, SIGNAL(requestFinished(int, bool)), this, SLOT(httpRequestFinished(int, bool)));
  connect(m_pHttp, SIGNAL(responseHeaderReceived(const QHttpResponseHeader &)), this, SLOT(readResponseHeader(const QHttpResponseHeader &)));

  pushButton_NewTriangle->setStyleSheet( "QPushButton { background: rgb(0, 150, 0); color: rgb(255, 255, 255) }" );
  pushButton_NewRectangle->setStyleSheet( "QPushButton { background: rgb(0, 150, 0); color: rgb(255, 255, 255) }" );
  pushButton_DelPoly_SimpleUI->setStyleSheet( "QPushButton { background: rgb(150, 0, 0); color: rgb(255, 255, 255) }" );

// DIRTY HACKS BEGIN

#ifdef DETECTED_OS_APPLE
    // hide edit menu as the preference item (the only item in edit right now) is magically moved on OS X to the program menu
    menu_File->addAction(actionSettings);
    menu_Edit->removeAction(actionSettings);
    delete menu_Edit;
#else
    // hide progress labels on systems that support text on top of the actual progressbars
    frame_24->setVisible(false);
    frame_23->setVisible(false);
#endif

	/// \todo remove this once we figured out how to do fullscreen 
	actionGo_Fullscreen->setVisible(false);

  //pushButton_ToggleBasic->setVisible(false);
// DIRTY HACKS END
}

// ******************************************
// Workspace
// ******************************************

void MainWindow::SetupWorkspaceMenu() {

/// \todo Implement the functionality of the other workspaces

//  menu_Workspace->addAction(dockWidget_Tools->toggleViewAction());
//  menu_Workspace->addAction(dockWidget_Filters->toggleViewAction());
//  menu_Workspace->addSeparator();

  radioButton_ToolsLock->setVisible(false);
  radioButton_FiltersLock->setVisible(false);

  menu_Workspace->addAction(dockWidget_RenderOptions->toggleViewAction());
  dockWidget_RenderOptions->toggleViewAction()->setShortcut(tr("Ctrl+Alt+1"));
  menu_Workspace->addAction(dockWidget_ProgressView->toggleViewAction());
  dockWidget_ProgressView->toggleViewAction()->setShortcut(tr("Ctrl+Alt+2"));
  menu_Workspace->addSeparator();
  menu_Workspace->addAction(dockWidget_1DTrans->toggleViewAction());
  dockWidget_1DTrans->toggleViewAction()->setShortcut(tr("Ctrl+Alt+3"));
  menu_Workspace->addAction(dockWidget_2DTrans->toggleViewAction());
  dockWidget_2DTrans->toggleViewAction()->setShortcut(tr("Ctrl+Alt+4"));
  menu_Workspace->addAction(dockWidget_IsoSurface->toggleViewAction());
  dockWidget_IsoSurface->toggleViewAction()->setShortcut(tr("Ctrl+Alt+5"));
  menu_Workspace->addSeparator();
  menu_Workspace->addAction(dockWidget_LockOptions->toggleViewAction());
  dockWidget_LockOptions->toggleViewAction()->setShortcut(tr("Ctrl+Alt+6"));
  menu_Workspace->addAction(dockWidget_Recorder->toggleViewAction());
  dockWidget_Recorder->toggleViewAction()->setShortcut(tr("Ctrl+Alt+7"));
  menu_Workspace->addAction(dockWidget_Stereo->toggleViewAction());
  dockWidget_Stereo->toggleViewAction()->setShortcut(tr("Ctrl+Alt+8"));
  menu_Workspace->addAction(dockWidget_Information->toggleViewAction());
  dockWidget_Information->toggleViewAction()->setShortcut(tr("Ctrl+Alt+9"));
  menu_Workspace->addAction(dockWidget_Lighting->toggleViewAction());
  dockWidget_Lighting->toggleViewAction()->setShortcut(tr("Ctrl+Alt+0"));


  menu_Help->addAction(dockWidget_Debug->toggleViewAction());
  dockWidget_Debug->toggleViewAction()->setShortcut(tr("Ctrl+Alt+D"));
}

void MainWindow::ClearWSMRUList()
{
  QSettings settings;
  QStringList files;
  files.clear();
  settings.setValue("Menu/WS_MRU", files);

  UpdateMRUActions();
}

void MainWindow::AddFileToWSMRUList(const QString &fileName)
{
  if (m_bScriptMode || fileName == "") return;

  QSettings settings;
  QStringList files = settings.value("Menu/WS_MRU").toStringList();

  files.removeAll(fileName);
  files.prepend(fileName);
  while ((unsigned int)(files.size()) > ms_iMaxRecentFiles)
    files.removeLast();

  settings.setValue("Menu/WS_MRU", files);

  UpdateWSMRUActions();
}


void MainWindow::UpdateWSMRUActions()
{
  QSettings settings;
  QStringList files = settings.value("Menu/WS_MRU").toStringList();

  int numRecentFiles = qMin(files.size(), (int)ms_iMaxRecentFiles);

  for (int i = 0; i < numRecentFiles; ++i) {
    QString text = tr("&%1 %2").arg(i + 1).arg(QFileInfo(files[i]).fileName());
    m_recentWSFileActs[i]->setText(text);
    m_recentWSFileActs[i]->setData(files[i]);
    m_recentWSFileActs[i]->setVisible(true);
    QString shortcut = tr("Ctrl+Shift+%1").arg(i + 1);
    m_recentWSFileActs[i]->setShortcut(QKeySequence(shortcut));
  }

  for (unsigned int j = numRecentFiles; j < ms_iMaxRecentFiles; ++j)
    m_recentWSFileActs[j]->setVisible(false);
}


void MainWindow::InitDockWidget(QDockWidget * v) const {
  v->setVisible(false);
  v->setFloating(true);
  v->resize(v->minimumSize());
}

void MainWindow::InitAllWorkspaces() {
  InitDockWidget(dockWidget_Tools); // no used at the moment
  InitDockWidget(dockWidget_Filters); // no used at the moment
  InitDockWidget(dockWidget_Lighting);
  InitDockWidget(dockWidget_Information);
  InitDockWidget(dockWidget_Recorder);
  InitDockWidget(dockWidget_LockOptions);
  InitDockWidget(dockWidget_RenderOptions);
  InitDockWidget(dockWidget_ProgressView);
  InitDockWidget(dockWidget_1DTrans);
  InitDockWidget(dockWidget_2DTrans);
  InitDockWidget(dockWidget_IsoSurface);
  InitDockWidget(dockWidget_Debug);
  InitDockWidget(dockWidget_Stereo);
}


bool MainWindow::LoadWorkspace() {
  QSettings settings;
  QString strLastDir = settings.value("Folders/LoadWorkspace", ".").toString();

  QFileDialog::Options options;
#ifdef DETECTED_OS_APPLE
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
#ifdef DETECTED_OS_APPLE
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

bool MainWindow::LoadDefaultWorkspace() {
  QSettings settings;
  if (settings.contains("Geometry/DockGeometry"))
    return restoreState( settings.value("Geometry/DockGeometry").toByteArray() );
  else 
    return false;
}

void MainWindow::SaveDefaultWorkspace() {
  QSettings settings;
  settings.setValue("Geometry/DockGeometry", this->saveState() );
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

  if (bOK) AddFileToWSMRUList(strFilename);

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
  if (ActiveSubWindow()) 
    ActiveSubWindow()->resize(iSizeX, iSizeY);
  else
    if (mdiArea->activeSubWindow()) 
      mdiArea->activeSubWindow()->resize(iSizeX, iSizeY);
}

void MainWindow::CloseCurrentView() {
  if (ActiveSubWindow()) 
    ActiveSubWindow()->close();
  else
    if (mdiArea->activeSubWindow()) 
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
    m_MasterController.DebugOut()->Message("MainWindow::CheckRenderwindowFitness","Renderwindow healthy.");

    if (bIfNotOkShowMessageAndCloseWindow && !bIsOK) {
      m_MasterController.DebugOut()->Error("MainWindow::CheckRenderwindowFitness","Unable to initialize the render window, see previous error messages for details.");

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
  }
  return false;
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
                                     m_bDisableBorder, m_bNoRCClipplanes,
                                     m_glShareWidget, fmt, this, 0);
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
                                   m_bDisableBorder, m_bNoRCClipplanes,
                                   m_glShareWidget, fmt, this, 0);
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
    m_MasterController.DebugOut()->Message("MainWindow::CreateNewRenderWindow","Calling RenderWindowActive");
    QCoreApplication::processEvents();
#ifdef DETECTED_OS_APPLE
    // HACK: For some reason on the Mac we need to set the active sub window,
    // re-process events, and then call our activation function ... doesn't
    // seem to happen automagically.
    QList<QMdiSubWindow *>::iterator iter;
    for(iter = mdiArea->subWindowList().begin();
        iter != mdiArea->subWindowList().end(); ++iter) {
      if(renderWin->GetQtWidget() == (*iter)->widget()) {
        mdiArea->setActiveSubWindow(*iter);
        break;
      }
    }
    QCoreApplication::processEvents();
#endif
    if(!renderWin->IsRenderSubsysOK()) {
      T_ERROR("Could not initialize render window!");
    } else {
      // Despite us registering it as the appropriate callback, Qt doesn't like
      // to call this for us w/in a timeframe which is useful.  Awesome.  So we
      // do it manually, to make sure the window is initialized before we
      // start using it.
      MESSAGE("Attempting to force QGL initialization");
      renderWin->GetQtWidget()->repaint();
    }
  }

  return renderWin;
}


void MainWindow::RenderWindowActive(RenderWindow* sender) {
  // to make sure we are only calling this code if the renderwindow changes,
  // and not just if the same window gets reactivated, keep track of the
  // last active window
  if (m_pActiveRenderWin != sender) {
    m_pActiveRenderWin = sender;
    m_MasterController.DebugOut()->
      Message("MainWindow::RenderWindowActive",
        "ACK that %s is now active",
        sender->GetDatasetName().toStdString().c_str());

    if (!CheckRenderwindowFitness(m_pActiveRenderWin)) {
      QMdiSubWindow* w = ActiveSubWindow();
      if (w) w->close();
      return;
    }

    MESSAGE("Getting 1D Transfer Function.");
    m_1DTransferFunction->
      SetData(&sender->GetRenderer()->GetDataset().Get1DHistogram(),
              sender->GetRenderer()->Get1DTrans());
    m_1DTransferFunction->update();
    MESSAGE("Getting 2D Transfer Function.");
    m_2DTransferFunction->
      SetData(&sender->GetRenderer()->GetDataset().Get2DHistogram(),
              sender->GetRenderer()->Get2DTrans());
    m_2DTransferFunction->update();

    MESSAGE("Getting other Renderwindow parameters.");
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

    EnableStereoWidgets();
    checkBox_Stereo->setChecked(m_pActiveRenderWin->GetRenderer()->GetStereo());
    horizontalSlider_EyeDistance->setValue(int(m_pActiveRenderWin->GetRenderer()->GetStereoEyeDist()*100));
    horizontalSlider_FocalLength->setValue(int(m_pActiveRenderWin->GetRenderer()->GetStereoFocalLength()*10));

    checkBox_Lighting->setChecked(m_pActiveRenderWin->GetRenderer()->GetUseLighting());
    SetSampleRateSlider(int(m_pActiveRenderWin->GetRenderer()->GetSampleRateModifier()*100));
    int iRange = int(m_pActiveRenderWin->GetDynamicRange())-1;
    SetIsoValueSlider(int(m_pActiveRenderWin->GetRenderer()->GetIsoValue()*iRange), iRange);

    DOUBLEVECTOR3 vfRescaleFactors =  m_pActiveRenderWin->GetRenderer()->GetRescaleFactors();
    doubleSpinBox_RescaleX->setValue(vfRescaleFactors.x);
    doubleSpinBox_RescaleY->setValue(vfRescaleFactors.y);
    doubleSpinBox_RescaleZ->setValue(vfRescaleFactors.z);

    SetToggleGlobalBBoxLabel(m_pActiveRenderWin->GetRenderer()->GetGlobalBBox());
    SetToggleLocalBBoxLabel(m_pActiveRenderWin->GetRenderer()->GetLocalBBox());

    AbstrRenderer *const ren = m_pActiveRenderWin->GetRenderer();
    SetToggleClipEnabledLabel(ren->ClipPlaneEnabled());
    SetToggleClipShownLabel(ren->ClipPlaneShown());
    SetToggleClipLockedLabel(ren->ClipPlaneLocked());

    ClearProgressViewAndInfo();

    ToggleClearViewControls(iRange);
    UpdateLockView();

    groupBox_ClipPlane->setVisible(m_pActiveRenderWin->GetRenderer()->CanDoClipPlane());

    lineEdit_DatasetName->setText(QFileInfo(m_pActiveRenderWin->GetDatasetName()).fileName());
    UINT64VECTOR3 vSize = m_pActiveRenderWin->GetRenderer()->GetDataset().GetDomainSize();
    UINT64 iBitWidth = m_pActiveRenderWin->GetRenderer()->GetDataset().GetBitWidth();
    QString strSize = tr("%1 x %2 x %3 (%4bit)").arg(vSize.x).arg(vSize.y).arg(vSize.z).arg(iBitWidth);
    lineEdit_MaxSize->setText(strSize);
    UINT64 iLevelCount = m_pActiveRenderWin->GetRenderer()->GetDataset().GetLODLevelCount();
    QString strLevelCount = tr("%1").arg(iLevelCount);
    lineEdit_MaxLODLevels->setText(strLevelCount);

    horizontalSlider_maxLODLimit->setMaximum(iLevelCount-1);
    horizontalSlider_minLODLimit->setMaximum(iLevelCount-1);

    UINTVECTOR2 iLODLimits = m_pActiveRenderWin->GetRenderer()->GetLODLimits();

    horizontalSlider_minLODLimit->setValue(iLODLimits.x);
    horizontalSlider_maxLODLimit->setValue(iLODLimits.y);

    UpdateMinMaxLODLimitLabel();

    UpdateColorWidget();
  }
}

void MainWindow::UpdateMinMaxLODLimitLabel() {
  label_minLODLimit->setText(tr("Limit Minimum Quality by skipping the lowest %1 level(s)").arg(horizontalSlider_minLODLimit->value()));
  label_maxLODLimit->setText(tr("Limit Maximum Quality by not rendering the highest %1 level(s)").arg(horizontalSlider_maxLODLimit->value()));
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
  vfRescaleFactors.x = std::max<float>(0.001f,doubleSpinBox_RescaleX->value());
  vfRescaleFactors.y = std::max<float>(0.001f,doubleSpinBox_RescaleY->value());
  vfRescaleFactors.z = std::max<float>(0.001f,doubleSpinBox_RescaleZ->value());
  m_pActiveRenderWin->GetRenderer()->SetRescaleFactors(vfRescaleFactors);
}


void MainWindow::StereoDisabled() {
  checkBox_Stereo->setChecked(false);
}

void MainWindow::RenderWindowViewChanged(int iMode) {
  groupBox_MovieCapture->setEnabled(iMode == 0);
}

void MainWindow::EnableStereoWidgets() {
  checkBox_Stereo->setEnabled(true);
  horizontalSlider_EyeDistance->setEnabled(true);
  horizontalSlider_FocalLength->setEnabled(true);
}
void MainWindow::DisableStereoWidgets() {
  checkBox_Stereo->setEnabled(false);
  horizontalSlider_EyeDistance->setEnabled(false);
  horizontalSlider_FocalLength->setEnabled(false);
}

void MainWindow::RenderWindowClosing(RenderWindow* sender) {
  sender->GetQtWidget()->setEnabled(false);
  m_MasterController.DebugOut()->
    Message("MainWindow::RenderWindowClosing",
      "ACK that %s is now closing",
      sender->GetDatasetName().toStdString().c_str());

  RemoveAllLocks(sender);

  disconnect(sender->GetQtWidget(), SIGNAL(WindowActive(RenderWindow*)),  this, SLOT(RenderWindowActive(RenderWindow*)));
  disconnect(sender->GetQtWidget(), SIGNAL(WindowClosing(RenderWindow*)), this, SLOT(RenderWindowClosing(RenderWindow*)));
  disconnect(sender->GetQtWidget(), SIGNAL(RenderWindowViewChanged(int)), this, SLOT(RenderWindowViewChanged(int)));
  disconnect(sender->GetQtWidget(), SIGNAL(StereoDisabled()), this, SLOT(StereoDisabled()));

  m_1DTransferFunction->SetData(NULL, NULL);
  m_1DTransferFunction->update();
  m_2DTransferFunction->SetData(NULL, NULL);
  m_2DTransferFunction->update();

  DisableAllTrans();

  DisableStereoWidgets();

  ClearProgressViewAndInfo();
  
  UpdateColorWidget();
  UpdateLockView();

  m_pActiveRenderWin = NULL;
}


void MainWindow::ToggleRenderWindowView2x2() {
  if (m_pActiveRenderWin) m_pActiveRenderWin->ToggleRenderWindowView2x2();
}


void MainWindow::ToggleRenderWindowViewSingle() {
  if (m_pActiveRenderWin) m_pActiveRenderWin->ToggleRenderWindowViewSingle();
}

void MainWindow::CheckForRedraw() {
  m_pRedrawTimer->stop();
  for (int i = 0;i<mdiArea->subWindowList().size();i++) {
    QWidget* w = mdiArea->subWindowList().at(i)->widget();
    RenderWindow* r = WidgetToRenderWin(w);
    // It can happen that our window was created, yet GL initialization failed
    // so it never got into a valid state.  Then a Qt event can pop up before
    // the `invalid window' detection code gets reached, and in that event
    // we'll end up checking for redraw.
    // In short: this method can end up being called even if we don't have a
    // render window.
    if(r) {
      r->CheckForRedraw();
    }
  }
  m_pRedrawTimer->start(20);
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

void MainWindow::OpenRecentWSFile(){
  QAction *action = qobject_cast<QAction *>(sender());

  if (SysTools::FileExists(string(action->data().toString().toAscii()))) {
    if (action) LoadWorkspace(action->data().toString());
  } 
}

void MainWindow::UpdateMenus() {
  bool bHasMdiChild = mdiArea->subWindowList().size() > 0;
  actionExport_Dataset->setEnabled(bHasMdiChild);
  actionTransfer_to_ImageVis3D_Mobile_Device->setEnabled(bHasMdiChild);

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
  if (m_bScriptMode || fileName == "") return;

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
    QString shortcut = tr("Ctrl+%1").arg(i + 1);
    m_recentFileActs[i]->setShortcut(QKeySequence(shortcut));
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

void MainWindow::ShowWelcomeScreen() {
/*
  // This code should center the window in its parent, but for now we just let QT decide where to put the window
  QSize qSize = this->size();
  QPoint qPos = this->pos();
  QSize qWelcomeSize = m_pWelcomeDialog->size();
  QSize qTmp =  (qSize - qWelcomeSize) / 2.0f;
  QPoint qNewWelcomePos(qTmp.width(), qTmp.height());
  m_pWelcomeDialog->move(qPos+qNewWelcomePos );
*/

  m_pWelcomeDialog->SetShowAtStartup(m_bShowWelcomeScreen);
  m_pWelcomeDialog->ClearMRUItems();

  QSettings settings;
  QStringList files = settings.value("Menu/MRU").toStringList();
  int numRecentFiles = qMin(files.size(), (int)ms_iMaxRecentFiles);
  for (int i = 0; i < numRecentFiles; ++i) {
    QString text = tr("%1").arg(QFileInfo(files[i]).fileName());
    m_pWelcomeDialog->AddMRUItem(string(text.toAscii()), string(files[i].toAscii()));
  }

  m_pWelcomeDialog->setWindowIcon(windowIcon());
  m_pWelcomeDialog->show();
}


void MainWindow::DisplayMetadata() {

  if (m_pActiveRenderWin)  {
    const vector< pair <string, string > >& metadata = m_pActiveRenderWin->GetRenderer()->GetDataset().GetMetadata();

    if (metadata.size() > 0) {
      m_pMetadataDialog->setWindowIcon(windowIcon());
      m_pMetadataDialog->SetMetadata(metadata);
      m_pMetadataDialog->SetFilename(lineEdit_DatasetName->text());
      m_pMetadataDialog->show();
    } else {
      QMessageBox::information(this, "Metadata viewer", "This file does not contain metadata!");     
    }
  }
}