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

#include <fstream>
#include <iostream>
#include <string>

#include <QtCore/QSettings>
#include <QtCore/QMimeData>
#include <QtCore/QTimer>
#include <QtCore/QUrl>
#include <QtWidgets/QColorDialog>
#include <QtGui/QDropEvent>
#include <QtWidgets/QFileDialog>
#include <QtWidgets/QInputDialog>
#include <QtWidgets/QMdiSubWindow>
#include <QtWidgets/QMessageBox>

// TODO: restore http functionality
// #include <QtNetwork/QHttp>

#include "../Tuvok/Controller/Controller.h"
#include "../Tuvok/Basics/SysTools.h"
#include "../Tuvok/LuaScripting/LuaScripting.h"
#include "../Tuvok/LuaScripting/TuvokSpecific/LuaTuvokTypes.h"

#include "ImageVis3D.h"
#include "MDIRenderWin.h"
#include "BrowseData.h"
#include "RenderWindowGL.h"
#include "RenderWindowDX.h"
#include "../Tuvok/Renderer/RenderMesh.h"
#include "ScaleAndBiasDlg.h"
#include "PleaseWait.h"

using namespace std;

void MainWindow::dragEnterEvent(QDragEnterEvent* ev)
{
  ev->acceptProposedAction();
}

void MainWindow::dropEvent(QDropEvent* ev)
{
  if (!ev->mimeData()->hasUrls()) return;

  std::wstring filename;

  QList<QUrl> urllist = ev->mimeData()->urls();
  for(int i=0; i < urllist.size(); ++i) {
    std::wstring fn = std::wstring(urllist[i].path().toStdWString());

#ifdef DETECTED_OS_WINDOWS
    if (!fn.empty() && fn[0] == '/') fn = fn.substr(1);
#endif
    if(SysTools::FileExists(fn)) {
      filename = fn;
    } else {
      WARNING("Ignoring drop of %s: file does not exist.", SysTools::toNarrow(fn).c_str());
    }
  }

  if(filename == L"") {
    // we didn't find a valid filename
    ev->ignore();
    return;
  }

  MESSAGE("Got file '%s' from drop event.", SysTools::toNarrow(filename).c_str());
  if(SysTools::GetExt(filename) == L"1dt") {
    this->Transfer1DLoad(filename);
  } else {
    this->LoadDataset(filename);
  }
  ev->accept();
}

// ******************************************
// Window Geometry
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
    fileName = QString::fromStdWString(SysTools::CheckExt(fileName.toStdWString(), L"geo"));
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
    std::wstring stdString(strFilename.toStdWString());
    if (LoadGeometry(QString::fromStdWString(SysTools::GetFromResourceOnMac(stdString)),true, false)) {
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


void MainWindow::SetHistogramScale1D(int v) {
  m_1DTransferFunction->SetHistogramScale(v);

  if (!m_pActiveRenderWin) return;
  m_pActiveRenderWin->SetCurrent1DHistScale(float(v)/verticalSlider_1DTransHistScale->maximum());
}

void MainWindow::SetHistogramScale2D(int v) {
  m_2DTransferFunction->SetHistogramScale(v);

  if (!m_pActiveRenderWin) return;
  m_pActiveRenderWin->SetCurrent2DHistScale(float(v)/verticalSlider_2DTransHistScale->maximum());
}

void MainWindow::setupUi(QMainWindow *MainWindow) {

  Ui_MainWindow::setupUi(MainWindow);

  this->addDockWidget(Qt::BottomDockWidgetArea, m_pDebugScriptWindow);
  m_pDebugScriptWindow->setObjectName(QString::fromUtf8("DebugScriptWindow"));

  SetTitle();

  m_1DTransferFunction =
    new Q1DTransferFunction(m_MasterController, frame_1DTrans);
  verticalLayout_1DTrans->addWidget(m_1DTransferFunction);

  Populate1DTFLibList();

  m_2DTransferFunction =
    new Q2DTransferFunction(m_MasterController, frame_2DTrans);
  verticalLayout_2DTrans->addWidget(m_2DTransferFunction);

  m_pQLightPreview =
    new QLightPreview(frame_lightPreview);
  horizontalLayout_lightPreview->addWidget(m_pQLightPreview);

  connect(m_pQLightPreview, SIGNAL(lightMoved()), this, SLOT(LightMoved()));

  connect(verticalSlider_1DTransHistScale, SIGNAL(valueChanged(int)),
    this, SLOT(SetHistogramScale1D(int)));
  connect(verticalSlider_2DTransHistScale, SIGNAL(valueChanged(int)),
    this, SLOT(SetHistogramScale2D(int)));


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
  this->horizontalLayout_10->addWidget(m_glShareWidget);

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

  pushButton_NewTriangle->setStyleSheet( "QPushButton { background: rgb(0, 150, 0); color: rgb(255, 255, 255) }" );
  pushButton_NewRectangle->setStyleSheet( "QPushButton { background: rgb(0, 150, 0); color: rgb(255, 255, 255) }" );
  pushButton_DelPoly_SimpleUI->setStyleSheet( "QPushButton { background: rgb(150, 0, 0); color: rgb(255, 255, 255) }" );

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

// DIRTY HACKS BEGIN
  /// \todo remove this once we figured out how to do fullscreen
  actionGo_Fullscreen->setVisible(false);
// DIRTY HACKS END
  ResetTimestepUI();
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

  if(m_MasterController.ExperimentalFeatures()) {
    menu_Workspace->addAction(dockWidget_Time->toggleViewAction());
    dockWidget_Time->toggleViewAction()->setShortcut(tr("Ctrl-Alt-t"));
  }
  /// @todo FIXME need a shortcut for timestep dockWidget
  //dockWidget_IsoSurface->toggleViewAction()->setShortcut(tr("Ctrl+Alt+5"));

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

  menu_Help->addAction(m_pDebugScriptWindow->toggleViewAction());
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
  v->move(100, 100);
  v->setVisible(false);
  v->setFloating(true);
  v->resize(v->minimumSize());
#ifdef DETECTED_OS_APPLE
  // Ahh, Qt, how I love thee; let me count the ways...
  //
  // Dock widgets have no frames.  Yet Qt 4.6.0 does position based on
  // the window size with a frame.  Further, Qt 4.6.0 defaults to (0,0)
  // for window positions which aren't otherwise set.
  //
  // In effect, this means that the default position for our dock
  // widgets is so high up in the screen that the top few pixels of the
  // widget are actually *under* the Mac default menubar.  This means
  // the dock widgets are immobile, because you can't click anywhere to
  // drag them.  Maybe Apple will someday make a mighty mighty Mouse,
  // whose special power is to click under menubars...
  //
  // To make things extra-special, this only seems to happen when
  // Qt is compiled against Carbon.  Moving windows a tad inward
  // isn't necessarily bad, though, so we don't bother checking for
  // Cocoa.  Eventually, we'll be doing Cocoa-only binaries, and should
  // probably remove this code so that it doesn't make anybody else
  // vomit.
  if(v->pos().y() < 25) {
    v->move(std::max(v->pos().x(), 5), 25);
  }
#endif
}

void MainWindow::InitAllWorkspaces() {
  InitDockWidget(dockWidget_Lighting);
  InitDockWidget(dockWidget_Information);
  InitDockWidget(dockWidget_Recorder);
  InitDockWidget(dockWidget_LockOptions);
  InitDockWidget(dockWidget_RenderOptions);
  InitDockWidget(dockWidget_ProgressView);
  InitDockWidget(dockWidget_1DTrans);
  InitDockWidget(dockWidget_2DTrans);
  InitDockWidget(dockWidget_IsoSurface);
  InitDockWidget(dockWidget_Time);
  InitDockWidget(dockWidget_Debug);
  InitDockWidget(dockWidget_Stereo);
  InitDockWidget(m_pDebugScriptWindow);
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
    fileName = QString::fromStdWString(SysTools::CheckExt(fileName.toStdWString(), L"wsp"));
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
    std::wstring stdString(strFilename.toStdWString());
    QString qfilename = QString::fromStdWString(SysTools::GetFromResourceOnMac(stdString));
    if (LoadWorkspace(qfilename, true, false)) {
        m_strCurrentWorkspaceFilename = qfilename;
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

void MainWindow::ResetToDefaultWorkspace() {
    InitAllWorkspaces();
}

// ******************************************
// Render Windows
// ******************************************


void MainWindow::PlaceCurrentView(int iPosX, int iPosY) {
  shared_ptr<LuaScripting> ss(m_MasterController.LuaScript());
  if (ActiveSubWindow()) {
    ActiveSubWindow()->move(iPosX, iPosY);
  } else {
    if (mdiArea->activeSubWindow()) {
      mdiArea->activeSubWindow()->move(iPosX, iPosY);
    }
  }
}

void MainWindow::ResizeCurrentView(int iSizeX, int iSizeY) {
  shared_ptr<LuaScripting> ss(m_MasterController.LuaScript());
  if (ActiveSubWindow()) {
    UINTVECTOR2 renderSize = m_pActiveRenderWin->GetRendererSize();
    UINTVECTOR2 windowSize(ActiveSubWindow()->size().width(), 
                           ActiveSubWindow()->size().height());

    UINTVECTOR2 winDecoSize = windowSize-renderSize;
    ActiveSubWindow()->resize(iSizeX+winDecoSize.x, iSizeY+winDecoSize.y);
  } else {
    if (mdiArea->activeSubWindow()) {
      UINTVECTOR2 renderSize = 
          WidgetToRenderWin(mdiArea->activeSubWindow()->widget())->GetRendererSize();
      UINTVECTOR2 windowSize(mdiArea->activeSubWindow()->size().width(), 
                             mdiArea->activeSubWindow()->size().height());

      UINTVECTOR2 winDecoSize = windowSize-renderSize;
      mdiArea->activeSubWindow()->resize(iSizeX+winDecoSize.x, iSizeY+winDecoSize.y);
    }
  }
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

  if(renderWin == NULL) {
    return;
  }

  renderWin->CloneViewState(m_pActiveRenderWin);
  renderWin->CloneRendermode(m_pActiveRenderWin);

  if (m_bAutoLockClonedWindow)
    for (size_t i = 0;i<RenderWindow::ms_iLockCount;i++) SetLock(i, renderWin, m_pActiveRenderWin);

  QMdiSubWindow* pActiveWin = mdiArea->activeSubWindow(); // as "show" toggles the active renderwin we need to remeber it
  renderWin->GetQtWidget()->show();
  RenderWindowActive(renderWin);
  mdiArea->activeSubWindow()->resize(pActiveWin->size().width(),
                                     pActiveWin->size().height());
  SetTagVolume();

  CheckForMeshCapabilities(true);
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


void MainWindow::LuaPlaceActiveWindow(const UINTVECTOR2& position) {
  PlaceCurrentView(position.x, position.y);
}

void MainWindow::LuaResizeActiveWindow(const UINTVECTOR2& newSize) {
  ResizeCurrentView(newSize.x, newSize.y);
}

RenderWindow* MainWindow::LuaCreateNewWindow(std::wstring dataset) {
  std::vector<std::wstring> fileList;
  fileList.push_back(dataset);
  return LuaLoadDatasetInternal(fileList, std::wstring(), false);
}

RenderWindow* MainWindow::CreateNewRenderWindow(QString dataset) {
  static unsigned int iCounter = 0;
  RenderWindow *renderWin;

  #if defined(_WIN32) && defined(USE_DIRECTX)
    if (m_eVolumeRendererType == MasterController::DIRECTX_SBVR ||
        m_eVolumeRendererType == MasterController::DIRECTX_RAYCASTER ||
        m_eVolumeRendererType == MasterController::DIRECTX_2DSBVR ||
        m_eVolumeRendererType == MasterController::DIRECTX_GRIDLEAPER) {
      renderWin = new RenderWindowDX(m_MasterController, m_eVolumeRendererType, dataset,
                                     iCounter++, m_bPowerOfTwo, m_bDownSampleTo8Bits,
                                     m_bDisableBorder, this, 0);
    } else {
      QGLFormat fmt;
      fmt.setRgba(true);
      fmt.setAlpha(true);
      renderWin = new RenderWindowGL(m_MasterController, m_eVolumeRendererType, dataset,
                                     iCounter++, m_bPowerOfTwo, m_bDownSampleTo8Bits,
                                     m_bDisableBorder,
                                     m_glShareWidget, fmt, this, 0);
    }
  #else
    if (m_eVolumeRendererType == MasterController::DIRECTX_SBVR ||
        m_eVolumeRendererType == MasterController::DIRECTX_RAYCASTER ||
        m_eVolumeRendererType == MasterController::DIRECTX_2DSBVR ||
        m_eVolumeRendererType == MasterController::DIRECTX_GRIDLEAPER) {
      ShowInformationDialog( "No DirectX Support", "The system was unable to open a DirectX 10 render window, falling back to OpenGL. Please check your settings.");
      m_MasterController.DebugOut()->Message("MainWindow::CreateNewRenderWindow","The system was unable to open a DirectX 10 render window, falling back to OpenGL. Please check your settings.");

      if (m_eVolumeRendererType == MasterController::DIRECTX_SBVR)
        m_eVolumeRendererType = MasterController::OPENGL_SBVR;
      else if (m_eVolumeRendererType == MasterController::DIRECTX_RAYCASTER)
        m_eVolumeRendererType = MasterController::OPENGL_RAYCASTER;
      else if (m_eVolumeRendererType == MasterController::DIRECTX_2DSBVR)
        m_eVolumeRendererType = MasterController::OPENGL_2DSBVR;
      else
        m_eVolumeRendererType = MasterController::OPENGL_GRIDLEAPER;
    }
    // if they're using the basic configuration, choose a renderer for them.
    // We can't just choose this here because we need to query some OGL
    // parameters to figure this out.
    if(m_bAdvancedSettings == false) {
      m_eVolumeRendererType = MasterController::OPENGL_CHOOSE;
    }
    QGLFormat fmt;
    fmt.setAlpha(true);
    fmt.setRgba(true);
    renderWin = new RenderWindowGL(m_MasterController, m_eVolumeRendererType, dataset,
                                   iCounter++, m_bPowerOfTwo, m_bDownSampleTo8Bits,
                                   m_bDisableBorder,
                                   m_glShareWidget, fmt, this, 0);
  #endif

  if (m_pActiveRenderWin != renderWin && !renderWin->IsRenderSubsysOK()) {
    delete renderWin;
    T_ERROR("Could not initialize render window!");
    return NULL;
  }

  connect(renderWin->GetQtWidget(), SIGNAL(WindowActive(RenderWindow*)),
          this, SLOT(RenderWindowActive(RenderWindow*)));
  connect(renderWin->GetQtWidget(), SIGNAL(WindowClosing(RenderWindow*)),
          this, SLOT(RenderWindowClosing(RenderWindow*)));
  connect(renderWin->GetQtWidget(), SIGNAL(RenderWindowViewChanged(int)),
          this, SLOT(RenderWindowViewChanged(int)));
  connect(renderWin->GetQtWidget(), SIGNAL(StereoDisabled()),
          this, SLOT(StereoDisabled()));

  mdiArea->addSubWindow(new MDIRenderWin(m_MasterController, renderWin));
  renderWin->InitializeContext();

  if (m_pActiveRenderWin != renderWin && !renderWin->IsRenderSubsysOK()) {
    delete renderWin;
    T_ERROR("Could not initialize render window context!");
    return NULL;
  }

  m_pLastLoadedRenderWin = renderWin;

  ApplySettings(renderWin);

  QCoreApplication::processEvents();

  return renderWin;
}


void MainWindow::RenderWindowActive(RenderWindow* sender) {
  // to make sure we are only calling this code if the renderwindow changes,
  // and not just if the same window gets reactivated, keep track of the
  // last active window
  if(m_pActiveRenderWin == sender) {
    return;
  }

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
  shared_ptr<LuaScripting> ss = m_MasterController.LuaScript();
  LuaClassInstance ds = sender->GetRendererDataset();
  std::pair<double,double> range = 
      ss->cexecRet<std::pair<double,double>>(ds.fqName() + ".getRange");
  shared_ptr<const Histogram1D> hist1D = 
      ss->cexecRet<shared_ptr<const Histogram1D>>(
          ds.fqName() + ".get1DHistogram");
  LuaClassInstance tf1d = sender->GetRendererTransferFunction1D();
  m_1DTransferFunction->SetData(hist1D,
                                static_cast<unsigned int>(range.second-range.first),
                                tf1d);
  m_1DTransferFunction->update();

  MESSAGE("Getting 2D Transfer Function.");
  shared_ptr<const Histogram2D> hist2D = 
      ss->cexecRet<shared_ptr<const Histogram2D>>(
          ds.fqName() + ".get2DHistogram");
  LuaClassInstance tf2d = sender->GetRendererTransferFunction2D();
  m_2DTransferFunction->SetData(hist2D, tf2d);
  m_2DTransferFunction->update();

  MESSAGE("Getting other Renderwindow parameters.");
  AbstrRenderer::ERenderMode e = m_pActiveRenderWin->GetRenderMode();

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
  checkBox_Stereo->setChecked(sender->GetRendererStereoEnabled());
  horizontalSlider_EyeDistance->setValue(int(sender->GetRendererStereoEyeDist()*100));
  horizontalSlider_FocalLength->setValue(int(sender->GetRendererStereoFocalLength()*10));
  checkBox_EyeSwap->setChecked(sender->GetRendererStereoEyeSwap());

  switch (sender->GetRendererStereoMode()) {
    case AbstrRenderer::SM_RB       : radioButton_RBStereo->setChecked(true); break;
    case AbstrRenderer::SM_SCANLINE : radioButton_ScanlineStereo->setChecked(true); break;
    case AbstrRenderer::SM_SBS      : radioButton_SideBySide->setChecked(true); break;
    default                         : radioButton_AlterFrame->setChecked(true); break;
  }

  checkBox_Lighting->setChecked(sender->GetUseLighting());
  SetSampleRateSlider(int(sender->GetRendererSampleRateModifier()*100));
  SetFoVSlider(int(sender->GetRendererFoV()));
  int iRange = int(m_pActiveRenderWin->GetDynamicRange().second);
  SetIsoValueSlider(int(sender->GetRendererIsoValue()), iRange);

  DOUBLEVECTOR3 vfRescaleFactors = sender->GetRendererRescaleFactors();
  doubleSpinBox_RescaleX->setValue(vfRescaleFactors.x);
  doubleSpinBox_RescaleY->setValue(vfRescaleFactors.y);
  doubleSpinBox_RescaleZ->setValue(vfRescaleFactors.z);

  SetToggleGlobalBBoxLabel(sender->GetRendererGlobalBBox());
  SetToggleLocalBBoxLabel(sender->GetRendererLocalBBox());

  SetToggleClipEnabledLabel(sender->GetRendererClipPlaneEnabled());
  SetToggleClipShownLabel(sender->GetRendererClipPlaneShown());
  SetToggleClipLockedLabel(sender->GetRendererClipPlaneLocked());

  ClearProgressViewAndInfo();

  ToggleClearViewControls(iRange);
  UpdateLockView();


  UpdateExplorerView(true);
//  groupBox_ClipPlane->setVisible(sender->GetRendererCanDoClipPlane());

  lineEdit_DatasetName->setText(QFileInfo(m_pActiveRenderWin->
                                          GetDatasetName()).fileName());

  UINT64VECTOR3 vSize = ss->cexecRet<UINT64VECTOR3>(
          ds.fqName() + ".getDomainSize", size_t(0), size_t(0));
  uint64_t iBitWidth = ss->cexecRet<uint64_t>(ds.fqName() + ".getBitWidth");
  pair<double, double> pRange = ss->cexecRet<pair<double, double>>(
      ds.fqName() + ".getRange");
  uint64_t numDSTimesteps = ss->cexecRet<uint64_t>(ds.fqName() 
                                                   + ".getNumberOfTimesteps");
  uint64_t lodLevelCount = ss->cexecRet<uint64_t>(ds.fqName()
                                                  + ".getLODLevelCount");

  QString strSize = tr("%1 x %2 x %3 (%4bit)").arg(vSize.x).
                                               arg(vSize.y).
                                               arg(vSize.z).
                                               arg(iBitWidth);
  if (pRange.first<=pRange.second) {
    strSize = strSize + tr(" Min=%1 Max=%2").arg(uint64_t(pRange.first)).
                                             arg(uint64_t(pRange.second));
  }
  // -1: slider is 0 based, but label is not.
  SetTimestepSlider(static_cast<int>(sender->GetRendererTimestep()),
                    numDSTimesteps-1);
  UpdateTimestepLabel(static_cast<int>(sender->GetRendererTimestep()),
                      numDSTimesteps);

  lineEdit_MaxSize->setText(strSize);
  uint64_t iLevelCount = lodLevelCount;
  QString strLevelCount = tr("%1").arg(iLevelCount);
  lineEdit_MaxLODLevels->setText(strLevelCount);

  horizontalSlider_maxLODLimit->setMaximum(iLevelCount-1);
  horizontalSlider_minLODLimit->setMaximum(iLevelCount-1);

  UINTVECTOR2 iLODLimits = sender->GetRendererLODLimits();

  horizontalSlider_minLODLimit->setValue(iLODLimits.x);
  horizontalSlider_maxLODLimit->setValue(iLODLimits.y);

  UpdateMinMaxLODLimitLabel();

  UpdateColorWidget();

  UpdateTFScaleSliders();

  UpdateInterpolant();
}

void MainWindow::ToggleMesh() {
  if (!m_pActiveRenderWin) return;

  int iCurrent = listWidget_DatasetComponents->currentRow();
  if (iCurrent < 1 || iCurrent >= listWidget_DatasetComponents->count()) return;

  shared_ptr<RenderMesh> mesh = m_pActiveRenderWin->GetRendererMeshes()[iCurrent-1];
  mesh->SetActive(checkBox_ComponenEnable->isChecked());
  m_pActiveRenderWin->RendererSchedule3DWindowRedraws();
  ToggleClearViewControls();
}


void MainWindow::SetMeshDefOpacity() {
  if (!m_pActiveRenderWin) return;

  int iCurrent = listWidget_DatasetComponents->currentRow();
  if (iCurrent < 1 || iCurrent >= listWidget_DatasetComponents->count()) return;

  shared_ptr<RenderMesh> mesh = m_pActiveRenderWin->GetRendererMeshes()[iCurrent-1];
  
  FLOATVECTOR4 meshcolor = mesh->GetDefaultColor();

  float fSlideVal = horizontalSlider_MeshDefOpacity->value()/100.0f;

  if (fSlideVal != meshcolor.w) {
    meshcolor.w = fSlideVal;
    mesh->SetDefaultColor(meshcolor);
    m_pActiveRenderWin->RendererSchedule3DWindowRedraws();
  }
}

void MainWindow::SetMeshScaleAndBias() {
  if (!m_pActiveRenderWin) return;

  int iCurrent = listWidget_DatasetComponents->currentRow();
  if (iCurrent < 1 || iCurrent >= listWidget_DatasetComponents->count()) return;

  shared_ptr<RenderMesh> mesh = m_pActiveRenderWin->GetRendererMeshes()[iCurrent-1];

  FLOATVECTOR3 vCenter, vExtend;
  vCenter = m_pActiveRenderWin->GetRendererVolumeAABBCenter();
  vExtend = m_pActiveRenderWin->GetRendererVolumeAABBExtents();

  ScaleAndBiasDlg sbd(mesh,iCurrent-1,
                      vCenter-0.5f*vExtend,
                      vCenter+0.5f*vExtend,
                      this);
  connect(&sbd, SIGNAL(SaveTransform(ScaleAndBiasDlg*)), this, SLOT(SaveMeshTransform(ScaleAndBiasDlg*)));
  connect(&sbd, SIGNAL(RestoreTransform(ScaleAndBiasDlg*)), this, SLOT(RestoreMeshTransform(ScaleAndBiasDlg*)));
  connect(&sbd, SIGNAL(ApplyTransform(ScaleAndBiasDlg*)), this, SLOT(ApplMeshTransform(ScaleAndBiasDlg*)));
  connect(&sbd, SIGNAL(ApplyMatrixTransform(ScaleAndBiasDlg*)), this, SLOT(ApplyMatrixMeshTransform(ScaleAndBiasDlg*)));

  sbd.exec();

  disconnect(&sbd, SIGNAL(SaveTransform(ScaleAndBiasDlg*)), this, SLOT(SaveMeshTransform(ScaleAndBiasDlg*)));
  disconnect(&sbd, SIGNAL(RestoreTransform(ScaleAndBiasDlg*)), this, SLOT(RestoreMeshTransform(ScaleAndBiasDlg*)));
  disconnect(&sbd, SIGNAL(ApplyTransform(ScaleAndBiasDlg*)), this, SLOT(ApplMeshTransform(ScaleAndBiasDlg*)));
  disconnect(&sbd, SIGNAL(ApplyMatrixTransform(ScaleAndBiasDlg*)), this, SLOT(ApplyMatrixMeshTransform(ScaleAndBiasDlg*)));
}

void MainWindow::ApplMeshTransform(ScaleAndBiasDlg* sender) {
  if (!m_pActiveRenderWin || !sender) return;

  if (sender->GetApplyAll()) {
    auto meshes = m_pActiveRenderWin->GetRendererMeshes();
    for (size_t i = 0; i < meshes.size(); ++i) {
      meshes[i]->ScaleAndBias(sender->m_scaleVec, sender->m_biasVec);
    }
  } else {
    sender->m_pMesh->ScaleAndBias(sender->m_scaleVec, sender->m_biasVec);
  }
  m_pActiveRenderWin->RendererSchedule3DWindowRedraws();
}

void MainWindow::ApplyMatrixMeshTransform(ScaleAndBiasDlg* sender) {
  if (!m_pActiveRenderWin || !sender) return;

  if (sender->GetApplyAll()) {
    auto meshes = m_pActiveRenderWin->GetRendererMeshes();
    for (size_t i = 0; i < meshes.size(); ++i) {
      meshes[i]->Transform(sender->GetExpertTransform());
    }
  } else {
    sender->m_pMesh->Transform(sender->GetExpertTransform());
  }
  m_pActiveRenderWin->RendererSchedule3DWindowRedraws();
}

void MainWindow::RestoreMeshTransform(ScaleAndBiasDlg* sender) {
  if (!m_pActiveRenderWin || !sender) return;
  shared_ptr<LuaScripting> ss = m_MasterController.LuaScript();
  LuaClassInstance ds = m_pActiveRenderWin->GetRendererDataset();
  vector<shared_ptr<Mesh>> meshes = ss->cexecRet<vector<shared_ptr<Mesh>>>(
      ds.fqName() + ".getMeshes");

  if (sender->GetApplyAll()) {
    for (size_t i = 0; i < meshes.size(); ++i) {
      const shared_ptr<Mesh> m = meshes[i];
      m_pActiveRenderWin->RendererReloadMesh(i, m);
    }
  } else {
    const shared_ptr<Mesh> m = meshes[sender->m_index];
    m_pActiveRenderWin->RendererReloadMesh(sender->m_index, m);
  }
  m_pActiveRenderWin->RendererSchedule3DWindowRedraws();
}

void MainWindow::SaveMeshTransform(ScaleAndBiasDlg* sender) {
  if (!m_pActiveRenderWin || !sender) return;
  shared_ptr<LuaScripting> ss = m_MasterController.LuaScript();
  LuaClassInstance ds = m_pActiveRenderWin->GetRendererDataset();
  if (ss->cexecRet<LuaDatasetProxy::DatasetType>(
          ds.fqName() + ".getDSType") != LuaDatasetProxy::UVF) return;

  PleaseWaitDialog pleaseWait(this);
  pleaseWait.SetText("Saving transformation to UVF file...");
  pleaseWait.AttachLabel(&m_MasterController);

  m_pActiveRenderWin->SetDatasetIsInvalid(true);

  if (sender->GetApplyAll()) {
    auto meshes = m_pActiveRenderWin->GetRendererMeshes();
    bool error = false;
    for (size_t i = 0; i < meshes.size(); ++i) {
      const FLOATMATRIX4& m = meshes[i]->GetTransformFromOriginal();
      const FLOATVECTOR4& c = meshes[i]->GetDefaultColor();

      if (!ss->cexecRet<bool>(ds.fqName() + ".geomTransformToFile", i, m, c)) {
        error = true;
        break;
      } else {
        meshes[i]->DeleteTransformFromOriginal();
      }
    }
    pleaseWait.close();
    if (error) {
      ShowCriticalDialog("Transform Save Failed.",
        "Could not save geometry transform to the UVF file, "
        "maybe the file is write protected? For details please "
        "check the debug log ('Help | Debug Window').");
    }
  } else {
    const FLOATMATRIX4& m = m_pActiveRenderWin->GetRendererMeshes()[sender->m_index]->GetTransformFromOriginal();
    const FLOATVECTOR4& c = m_pActiveRenderWin->GetRendererMeshes()[sender->m_index]->GetDefaultColor();

    if (!ss->cexecRet<bool>(ds.fqName() + ".geomTransformToFile",
      static_cast<size_t>(sender->m_index), m, c)) {
      pleaseWait.close();
      ShowCriticalDialog("Transform Save Failed.",
        "Could not save geometry transform to the UVF file, "
        "maybe the file is write protected? For details please "
        "check the debug log ('Help | Debug Window').");
    } else {
      m_pActiveRenderWin->GetRendererMeshes()[sender->m_index]->DeleteTransformFromOriginal();
      pleaseWait.close();
    }
  }

  m_pActiveRenderWin->SetDatasetIsInvalid(false);
}

void MainWindow::SetMeshDefColor() {
  if (!m_pActiveRenderWin) return;

  int iCurrent = listWidget_DatasetComponents->currentRow();
  if (iCurrent < 1 || iCurrent >= listWidget_DatasetComponents->count()) return;

  shared_ptr<RenderMesh> mesh = m_pActiveRenderWin->GetRendererMeshes()[iCurrent-1];
  
  FLOATVECTOR4 meshcolor = mesh->GetDefaultColor();

  const int old_color[3] = {
    static_cast<int>(meshcolor.x * 255.f),
    static_cast<int>(meshcolor.y * 255.f),
    static_cast<int>(meshcolor.z * 255.f)
  };
  QColor prevColor(old_color[0], old_color[1], old_color[2]);
  QColor color = QColorDialog::getColor(prevColor, this);

  if (color.isValid()) {
    meshcolor.x = color.red()/255.0f;
    meshcolor.y = color.green()/255.0f;
    meshcolor.z = color.blue()/255.0f;
    mesh->SetDefaultColor(meshcolor);
    m_pActiveRenderWin->RendererSchedule3DWindowRedraws();
  }
}

void MainWindow::UpdateExplorerView(bool bRepopulateListBox) {
  if (!m_pActiveRenderWin) return;

  if (bRepopulateListBox) {
    listWidget_DatasetComponents->clear();
    
    shared_ptr<LuaScripting> ss(m_MasterController.LuaScript());
    LuaClassInstance ds = m_pActiveRenderWin->GetRendererDataset();
    
    QString voldesc = tr("Volume (%1)").arg(QString::fromStdWString(ss->cexecRet<std::wstring>(ds.fqName() + ".name")));
    listWidget_DatasetComponents->addItem(voldesc);

    size_t meshListSize = m_pActiveRenderWin->GetRendererMeshes().size();
    for (size_t i = 0;
         i<meshListSize;
         i++) {
      shared_ptr<const RenderMesh> mesh = 
          m_pActiveRenderWin->GetRendererMeshes()[i];
      QString meshdesc = tr("%1 (%2)").arg(mesh->GetMeshType() == Mesh::MT_TRIANGLES ? "Triangle Mesh" : "Lines").arg(SysTools::toNarrow(mesh->Name()).c_str());
      listWidget_DatasetComponents->addItem(meshdesc);
    }
    listWidget_DatasetComponents->setCurrentRow(0);
    ToggleClearViewControls();
  }

  int iCurrent = listWidget_DatasetComponents->currentRow();

  if (iCurrent < 0 || iCurrent >= listWidget_DatasetComponents->count()) {
    stackedWidget_componentInfo->setVisible(false);
    checkBox_ComponenEnable->setVisible(false);
    return;
  }

  stackedWidget_componentInfo->setVisible(true);
  checkBox_ComponenEnable->setVisible(true);

  if (iCurrent == 0) {
    page_Volume->setVisible(true);
    page_Geometry->setVisible(false);
    stackedWidget_componentInfo->setCurrentIndex(0);
  } else {
    page_Volume->setVisible(false);
    page_Geometry->setVisible(true);   
    stackedWidget_componentInfo->setCurrentIndex(1);

    shared_ptr<const RenderMesh> mesh = 
        m_pActiveRenderWin->GetRendererMeshes()[iCurrent-1];

    size_t iVerticesPerPoly = mesh->GetVerticesPerPoly();

    checkBox_ComponenEnable->setChecked(mesh->GetActive());
    size_t polycount = mesh->GetVertexIndices().size()/iVerticesPerPoly;
    size_t vertexcount = mesh->GetVertices().size();
    size_t normalcount = mesh->GetNormals().size();
    size_t texccordcount = mesh->GetTexCoords().size();
    size_t colorcount = mesh->GetColors().size();

    QString strPolycount = tr("%1").arg(polycount);
    lineEdit_MeshPolyCount->setText(strPolycount);
    QString strVertexcount = tr("%1").arg(vertexcount);
    lineEdit_VertexCount->setText(strVertexcount);
    QString strNormalcount = tr("%1").arg(normalcount);
    lineEdit_NormalCount->setText(strNormalcount);
    QString strTexccordcount = tr("%1").arg(texccordcount);
    lineEdit_TexCoordCount->setText(strTexccordcount);
    QString strColorcount = (colorcount != 0)
          ? tr("%1").arg(colorcount)
          : "using default color";
    lineEdit_ColorCount->setText(strColorcount);
    horizontalSlider_MeshDefOpacity->setVisible(mesh->GetMeshType() == Mesh::MT_TRIANGLES);
    label_MeshOpacity->setVisible(mesh->GetMeshType() == Mesh::MT_TRIANGLES);
    horizontalSlider_MeshDefOpacity->setValue(int(mesh->GetDefaultColor().w*100));
    frame_meshDefColor->setVisible(colorcount == 0);
  }
}

void MainWindow::UpdateMinMaxLODLimitLabel() {
  if (horizontalSlider_minLODLimit->value() == 1)
    label_minLODLimit->setText(tr("Limit Minimum Quality by skipping the lowest level"));
  else
    label_minLODLimit->setText(tr("Limit Minimum Quality by skipping the lowest %1 levels").arg(horizontalSlider_minLODLimit->value()));

  if (horizontalSlider_maxLODLimit->value() == 1)
    label_maxLODLimit->setText(tr("Limit Maximum Quality by not rendering the highest level"));
  else
    label_maxLODLimit->setText(tr("Limit Maximum Quality by not rendering the highest %1 levels").arg(horizontalSlider_maxLODLimit->value()));

}

void MainWindow::ToggleClearViewControls() {
  if (!m_pActiveRenderWin) return;

  if (m_pActiveRenderWin->SupportsClearView()) {
    checkBox_ClearView->setVisible(true);
    frame_ClearView->setVisible(true);
    checkBox_ClearView->setChecked(m_pActiveRenderWin->GetClearViewEnabled());
    label_CVDisableReason->setVisible(false);
  } else {
    checkBox_ClearView->setChecked(false);
    checkBox_ClearView->setVisible(false);
    frame_ClearView->setVisible(false);

    QString reason = tr("ClearView is disabled because %1").arg(
        m_pActiveRenderWin->GetRendererClearViewDisabledReason().c_str());
    label_CVDisableReason->setText(reason);
    label_CVDisableReason->setVisible(true);
    label_CVDisableReason->setWordWrap(true);
    m_pActiveRenderWin->RendererSchedule3DWindowRedraws();
  }
}

void MainWindow::ToggleClearViewControls(int iRange) {
  ToggleClearViewControls();

  SetFocusIsoValueSlider(
      int(m_pActiveRenderWin->GetRendererClearViewIsoValue()), iRange);
  SetFocusSizeValueSlider(
      99-int(m_pActiveRenderWin->GetRendererClearViewSize()*9.9f));
  SetContextScaleValueSlider(
      int(m_pActiveRenderWin->GetRendererClearViewContextScale()*10.0f));
  SetBorderSizeValueSlider(
      int(99-m_pActiveRenderWin->GetRendererClearViewBorderScale()));
}

void MainWindow::SetStayOpen(bool bStayOpenAfterScriptEnd) {
  m_bStayOpenAfterScriptEnd = bStayOpenAfterScriptEnd;
}

void MainWindow::SetRescaleFactors() {
  if (!m_pActiveRenderWin) return;
  DOUBLEVECTOR3 vfRescaleFactors;
  vfRescaleFactors.x = std::max<float>(0.001f,doubleSpinBox_RescaleX->value());
  vfRescaleFactors.y = std::max<float>(0.001f,doubleSpinBox_RescaleY->value());
  vfRescaleFactors.z = std::max<float>(0.001f,doubleSpinBox_RescaleZ->value());
  m_pActiveRenderWin->SetRendererRescaleFactors(vfRescaleFactors);
}


void MainWindow::StereoDisabled() {
  checkBox_Stereo->setChecked(false);
}

void MainWindow::RenderWindowViewChanged(int iMode) {
  groupBox_MovieCapture->setEnabled(iMode == 0);
}

void MainWindow::EnableStereoWidgets() {
  frame_Stereo->setEnabled(true);
  horizontalSlider_EyeDistance->setEnabled(true);
  horizontalSlider_FocalLength->setEnabled(true);
  frame_StereoMode->setEnabled(true);
}

void MainWindow::DisableStereoWidgets() {
  frame_Stereo->setEnabled(false);
  horizontalSlider_EyeDistance->setEnabled(false);
  horizontalSlider_FocalLength->setEnabled(false);
  frame_StereoMode->setEnabled(false);
}

void MainWindow::RenderWindowClosing(RenderWindow* sender) {
  sender->GetQtWidget()->setEnabled(false);
  m_MasterController.DebugOut()->
    Message("MainWindow::RenderWindowClosing",
      "ACK that %s is now closing",
      sender->GetDatasetName().toStdString().c_str());

  RemoveAllLocks(sender);

  disconnect(sender->GetQtWidget());

  m_1DTransferFunction->SetData(NULL, 10, LuaClassInstance());
  m_1DTransferFunction->update();
  m_2DTransferFunction->SetData(NULL, LuaClassInstance());
  m_2DTransferFunction->update();

  DisableAllTrans();

  DisableStereoWidgets();

  ClearProgressViewAndInfo();

  UpdateColorWidget();
  UpdateLockView();
  ResetTimestepUI();

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
    if(r && r->IsRenderSubsysOK()) {
      r->CheckForRedraw();
    }
  }
  m_pRedrawTimer->start(IV3D_TIMER_INTERVAL);
}

// ******************************************
// Menus
// ******************************************

void MainWindow::OpenRecentFile(){
  QAction *action = qobject_cast<QAction *>(sender());

  if(action == NULL) { T_ERROR("no sender?"); return; }
  if (SysTools::FileExists(action->data().toString().toStdWString())) {
    if (!LoadDataset(QStringList(action->data().toString()))) {
      if (m_bIgnoreLoadDatasetFailure == false) {
        ShowCriticalDialog("Render window initialization failed.",
                     "Could not open a render window!  This normally "
                     "means ImageVis3D does not support your GPU.  Please"
                     " check the debug log ('Help | Debug Window') for "
                     "errors, and/or use 'Help | Report an Issue' to "
                     "notify the ImageVis3D developers.");
      }
      m_bIgnoreLoadDatasetFailure = false;
    }
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

  if(action == NULL) { T_ERROR("no sender?"); return; }
  if (SysTools::FileExists(action->data().toString().toStdWString())) {
    LoadWorkspace(action->data().toString());
  }
}

void MainWindow::UpdateMenus() {
  bool bHasMdiChild = mdiArea->subWindowList().size() > 0;
  actionExport_Dataset->setEnabled(bHasMdiChild);
  actionExport_Image_Stack->setEnabled(bHasMdiChild);
  actionTransfer_to_ImageVis3D_Mobile_Device->setEnabled(bHasMdiChild);
  actionAdd_Geometry_to_Data_Set->setEnabled(bHasMdiChild);

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
  if (!m_bScriptMode) 
    QMessageBox::critical(this, strTitle, strMessage);
  else {
    std::string s = strTitle.toStdString() + ": " + strMessage.toStdString();
    T_ERROR(s.c_str());
  }
}

void MainWindow::ShowInformationDialog(QString strTitle, QString strMessage) {
  if (!m_bScriptMode) 
    QMessageBox::information(this, strTitle, strMessage);
  else {
    string s = strTitle.toStdString() + ": " + strMessage.toStdString();
    MESSAGE(s.c_str());
  }
}

void MainWindow::ShowWarningDialog(QString strTitle, QString strMessage) {
  if (!m_bScriptMode) 
    QMessageBox::warning(this, strTitle, strMessage);
  else {
    string s = strTitle.toStdString() + ": " + strMessage.toStdString();
    WARNING(s.c_str());
  }
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
    m_pWelcomeDialog->AddMRUItem(text.toStdWString(), files[i].toStdWString());
  }

  m_pWelcomeDialog->setWindowIcon(windowIcon());
  m_pWelcomeDialog->show();
}


void MainWindow::DisplayMetadata() {
if (m_pActiveRenderWin)  {
    shared_ptr<LuaScripting> ss(m_MasterController.LuaScript());
    LuaClassInstance ds = m_pActiveRenderWin->GetRendererDataset();
    const vector<pair<std::wstring, std::wstring>>& metadata =
        ss->cexecRet<vector<pair<std::wstring, std::wstring>>>(ds.fqName() + ".getMetadata");

    if(!metadata.empty()) {
      m_pMetadataDialog->setWindowIcon(windowIcon());
      m_pMetadataDialog->SetMetadata(metadata);
      m_pMetadataDialog->SetFilename(lineEdit_DatasetName->text());
      m_pMetadataDialog->show();
    } else {
      QMessageBox::information(this, "Metadata viewer", "This file does not contain metadata!");
    }
  }
}
