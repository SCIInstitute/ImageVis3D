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


//!    File   : ImageVis3D_FileHandling.fcpp
//!    Author : Jens Krueger
//!             SCI Institute
//!             University of Utah
//!    Date   : September 2008
//
//!    Copyright (C) 2008 SCI Institute

#include <fstream>
#include <iterator>
#include <string>
#include <sstream>

#include "ImageVis3D.h"
#include "BrowseData.h"

#include <QtCore/QSettings>
#include <QtCore/QTimer>
#include <QtGui/QFileDialog>
#include <QtGui/QMessageBox>

#include "../Tuvok/Basics/SysTools.h"
#include "../Tuvok/Controller/Controller.h"
#include "../Tuvok/IO/IOManager.h"
#include "../Tuvok/IO/TuvokIOError.h"
#include "../Tuvok/IO/uvfDataset.h"

#include "DebugOut/QTLabelOut.h"
#include "LODDlg.h"
#include "MergeDlg.h"
#include "PleaseWait.h"

using namespace std;
using namespace tuvok;

// ******************************************
// Dataset
// ******************************************

void MainWindow::LoadDataset() {
  QFileDialog::Options options;
#ifdef DETECTED_OS_APPLE
  options |= QFileDialog::DontUseNativeDialog;
#endif
  QString selectedFilter;

  QSettings settings;
  QString strLastDir = settings.value("Folders/LoadDataset", ".").toString();

  QString dialogString = m_MasterController.IOMan()->
                                            GetLoadDialogString().c_str();

  QStringList files;
  if(m_MasterController.ExperimentalFeatures()) {
    files = QFileDialog::getOpenFileNames(
              this, "Load Datasets", strLastDir, dialogString,
              &selectedFilter, options
            );
  } else {
    QString fileName = QFileDialog::getOpenFileName(this,
                 "Load Dataset", strLastDir,
                 dialogString, &selectedFilter, options);
    if (!fileName.isEmpty()) {
      files.append(fileName);
    }
  }

  if (!files.isEmpty()) {
    settings.setValue("Folders/LoadDataset",
                      QFileInfo(files[0]).absoluteDir().path());
    if(!LoadDataset(files)) {
        ShowCriticalDialog("Render window initialization failed.",
                     "Could not open a render window!  This normally "
                     "means ImageVis3D does not support your GPU.  Please"
                     " check the debug log ('Help | Debug Window') for "
                     "errors, and/or use 'Help | Report an Issue' to "
                     "notify the ImageVis3D developers.");
    }
  }
}


void MainWindow::ExportGeometry() {
 QFileDialog::Options options;
#ifdef DETECTED_OS_APPLE
  options |= QFileDialog::DontUseNativeDialog;
#endif
  QString selectedFilter;

  QSettings settings;
  QString strLastDir = settings.value("Folders/ExportMesh", ".").toString();

  QString fileName =
    QFileDialog::getSaveFileName(this, "Export Current Mesh to File",
         strLastDir,
         m_MasterController.IOMan()->GetGeoExportDialogString().c_str(),
         &selectedFilter, options);

  if (!fileName.isEmpty()) {
    settings.setValue("Folders/ExportMesh", QFileInfo(fileName).absoluteDir().path());
    string targetFileName = string(fileName.toAscii());

    // still a valid filename ext ?
    if (!m_MasterController.IOMan()->GetGeoConverterForExt(SysTools::ToLowerCase(SysTools::GetExt(string(fileName.toAscii()))),true,false)) {
      ShowCriticalDialog("Extension Error", "Unable to determine the file type from the file extension.");
      return;
    }

    int iCurrent = listWidget_DatasetComponents->currentRow();

    if (iCurrent < 1 || iCurrent >= listWidget_DatasetComponents->count()) {
      T_ERROR("No Mesh selected.");
      return;
    }

    if (!ExportGeometry(iCurrent-1,targetFileName)) {
      ShowCriticalDialog("Export Error", "Unable to export the mesh "
                         "to the selected file. "
                         "For details please check the debug log "
                         "('Help | Debug Window').");
      return;
    }
  }

}

bool MainWindow::ExportGeometry(size_t i, std::string strFilename) {
  if (!m_pActiveRenderWin) return false;
  const UVFDataset* currentDataset = dynamic_cast<UVFDataset*>(&(m_pActiveRenderWin->GetRenderer()->GetDataset()));
  if (!currentDataset) return false;

  PleaseWaitDialog pleaseWait(this);
  pleaseWait.SetText("Exporting Mesh...");
  pleaseWait.AttachLabel(&m_MasterController);

  const std::vector<Mesh*>& meshes = currentDataset->GetMeshes();
  return m_MasterController.IOMan()->ExportMesh(meshes[i], strFilename);
}

void MainWindow::RemoveGeometry() {
  if (!m_pActiveRenderWin) return;
  UVFDataset* currentDataset = dynamic_cast<UVFDataset*>(&(m_pActiveRenderWin->GetRenderer()->GetDataset()));
  if (!currentDataset) return;


  PleaseWaitDialog pleaseWait(this);
  pleaseWait.SetText("Removing Mesh from UVF file...");
  pleaseWait.AttachLabel(&m_MasterController);

  int iCurrent = listWidget_DatasetComponents->currentRow();

  if (iCurrent < 1 || iCurrent >= listWidget_DatasetComponents->count()) {
    T_ERROR("No Mesh selected.");
    return;
  }

  m_pActiveRenderWin->GetRenderer()->SetDatasetIsInvalid(true);

  if (!currentDataset->RemoveMesh(size_t(iCurrent-1))) {
    pleaseWait.close();
    ShowCriticalDialog("Mesh Removal Failed.",
             "Could not remove mesh from the UVF file, "
             "maybe the file is write protected? For details please "
             "check the debug log ('Help | Debug Window').");
  } else {
    m_pActiveRenderWin->GetRenderer()->RemoveMeshData(size_t(iCurrent-1));
    UpdateExplorerView(true);
    pleaseWait.close();
  }

    m_pActiveRenderWin->GetRenderer()->SetDatasetIsInvalid(false);
}

void MainWindow::AddGeometry(std::string filename) {
  if (!m_pActiveRenderWin) return;

  if(!m_pActiveRenderWin->GetRenderer()->SupportsMeshes()) {
      if(QMessageBox::Yes == 
        QMessageBox::question(NULL, 
                         "Mesh feature not supported in this renderer",
                         "You can add a mesh to this dataset but you will "
                         "not be able to see it until you switch to a renderer "
                         "that supports this feature e.g. the 3D slice "
                         "based volume renderer. Do you want to switch to "
                         "the 3D slice based volume renderer now?",
          QMessageBox::Yes, QMessageBox::No)) {
        QString dataset = m_pActiveRenderWin->GetDatasetName();
        CloseCurrentView();
        MasterController::EVolumeRendererType currentType = m_eVolumeRendererType;
        m_eVolumeRendererType = MasterController::OPENGL_SBVR;    
        LoadDataset(std::string(dataset.toAscii()));
        m_eVolumeRendererType = currentType;
      }
  }


  UVFDataset* currentDataset = dynamic_cast<UVFDataset*>(&(m_pActiveRenderWin->GetRenderer()->GetDataset()));
  if (!currentDataset) {
    ShowCriticalDialog("Mesh Import Failed.",
                 "Mesh Integration only supported for UVF datasets.");
    return;
  }

  PleaseWaitDialog pleaseWait(this);

  pleaseWait.SetText("Loading mesh, please wait  ...");
  pleaseWait.AttachLabel(&m_MasterController);

  Mesh* m = NULL;
  try {
    m = m_MasterController.IOMan()->LoadMesh(filename);
  } catch (const tuvok::io::DSOpenFailed& err) {
    WARNING("Conversion failed! %s", err.what());
  }

  if (m == NULL)  {
    ShowCriticalDialog("Mesh Import Failed.",
                 "Could not load mesh file. For details please "
                 "check the debug log ('Help | Debug Window').");
    return;
  }

  // make sure we have at least normals
  if (m->GetNormalIndices().empty()) {
    pleaseWait.SetText("Computing normals, please wait  ...");
    m->RecomputeNormals();
  }

  pleaseWait.SetText("Integrating Mesh into volume file, please wait  ...");

  m_pActiveRenderWin->GetRenderer()->SetDatasetIsInvalid(true);

  if (!currentDataset->AppendMesh(m)) {
    pleaseWait.close();
    ShowCriticalDialog("Mesh Import Failed.",
                 "Could not integrate the mesh into this UVF file. "
                 "For details please check the debug log "
                 "('Help | Debug Window').");
    delete m;
  } else {
    pleaseWait.SetText("Creating render resources, please wait  ...");
    m_pActiveRenderWin->GetRenderer()->ScanForNewMeshes();
    pleaseWait.close();
    UpdateExplorerView(true);
  }
  delete m;

  m_pActiveRenderWin->GetRenderer()->SetDatasetIsInvalid(false);
}

void MainWindow::AddGeometry() {
  QFileDialog::Options options;
#ifdef DETECTED_OS_APPLE
  options |= QFileDialog::DontUseNativeDialog;
#endif

  QSettings settings;
  QString selFilter;

  QString dialogString = m_MasterController.IOMan()->
                                            GetLoadGeoDialogString().c_str();

  QString strLastDir = settings.value("Folders/AddTriGeo", ".").toString();

  QString geoFile =
    QFileDialog::getOpenFileName(this, "Select Geometry File", strLastDir,
                                  dialogString,
                                  &selFilter, options);
  if(geoFile.isEmpty()) {
    return;
  }

  settings.setValue("Folders/AddTriGeo", QFileInfo(geoFile).absoluteDir().path());

  AddGeometry(string(geoFile.toAscii()));
}

void MainWindow::LoadDataset(std::string strFilename) {
  try {
    if(!LoadDataset(QStringList(strFilename.c_str())) != 0) {
      ShowCriticalDialog("Render window initialization failed.",
        "Could not open a render window!  This normally "
        "means ImageVis3D does not support your GPU.  Please"
        " check the debug log ('Help | Debug Window') for "
        "errors, and/or use 'Help | Report an Issue' to "
        "notify the ImageVis3D developers."
      );
    }
  } catch(const tuvok::io::DSParseFailed& ds) {
    std::ostringstream err;
    err << "Could not parse '" << ds.File() << "': " << ds.what() << "\n"
        << "This typically means the file has been corrupted.";
    ShowCriticalDialog("Could not parse file", err.str().c_str());
  } catch(const tuvok::io::DSVerificationFailed& ds) {
    std::ostringstream err;
    err << "The checksum for '" << ds.File() << "' is invalid: " << ds.what()
        << "\nThe file has been corrupted.";
    ShowCriticalDialog("Checksum failed", err.str().c_str());
  } catch(const tuvok::io::DSOpenFailed& ds) {
    std::ostringstream err;
    err << "Could not open '" << ds.File() << "': " << ds.what();
    ShowCriticalDialog("Could not open file", err.str().c_str());
  }
}

QString MainWindow::GetConvFilename(const QString& sourceName) {
  QFileDialog::Options options;
  #ifdef DETECTED_OS_APPLE
    options |= QFileDialog::DontUseNativeDialog;
  #endif
  QString selectedFilter;

  QSettings settings;
  QString strLastDir = settings.value("Folders/GetConvFilename", ".").toString();

  QString suggestedFileName = SysTools::ChangeExt(
                                SysTools::GetFilename(string(sourceName.toAscii())),
                                                      "uvf"
                              ).c_str();
  
  strLastDir = tr("%1/%2").arg(strLastDir).arg(suggestedFileName);

  QString targetFilename =
    QFileDialog::getSaveFileName(this, "Select filename for converted data",
                                 strLastDir, "Universal Volume Format (*.uvf)",
                                 &selectedFilter, options);

  if (!targetFilename.isEmpty()) {
    targetFilename = SysTools::CheckExt(string(targetFilename.toAscii()),
                                               "uvf").c_str();
    settings.setValue("Folders/GetConvFilename",
                      QFileInfo(targetFilename).absoluteDir().path());
  }

  return targetFilename;
}


bool MainWindow::LoadDataset(const std::vector< std::string >& strParams) {
  if (strParams.size() < 1 || strParams.size() > 2) {
    return false;
  }
  string inFile = strParams[0], convFile;
  if (strParams.size() == 1)  {
    convFile = SysTools::ChangeExt(inFile, "uvf");
  } else {
    convFile = strParams[1];
  }

  return LoadDataset(QStringList(inFile.c_str()), convFile.c_str(), false);
}

bool MainWindow::CheckForMeshCapabilities(bool bNoUserInteraction, QStringList files) {
  if (bNoUserInteraction) {
    if (m_pActiveRenderWin && 
      !m_pActiveRenderWin->GetRenderer()->SupportsMeshes() &&
      m_pActiveRenderWin->GetRenderer()->GetMeshes().size() > 0) {
        WARNING("This dataset contains mesh data but the current "
                "renderer does not supports rendering meshes. Mesh "
                "rendering is disabled until you switch to a renderer "
                "that supports this feature e.g. the 3D slice "
                "based volume renderer.");
        m_pActiveRenderWin->GetRenderer()->GetMeshes().clear();
        UpdateExplorerView(true);
    }
  } else {
    if (m_pActiveRenderWin && 
      !m_pActiveRenderWin->GetRenderer()->SupportsMeshes() &&
      m_pActiveRenderWin->GetRenderer()->GetMeshes().size() > 0) {
      m_pRedrawTimer->stop();
      if(QMessageBox::Yes == 
        QMessageBox::question(NULL, 
                         "Mesh feature not supported in this renderer",
                         "This dataset contains mesh data but the current "
                         "renderer does not support rendering meshes. Mesh "
                         "rendering is disabled until you switch to a renderer "
                         "that supports this feature e.g. the 3D slice "
                         "based volume renderer. Do you want to switch to "
                         "the 3D slice based volume renderer now?",
          QMessageBox::Yes, QMessageBox::No)) {
        CloseCurrentView();
        MasterController::EVolumeRendererType currentType = m_eVolumeRendererType;
        m_eVolumeRendererType = MasterController::OPENGL_SBVR;
        LoadDataset(files);
        m_eVolumeRendererType = currentType;
        return true;
      } else {
        m_pActiveRenderWin->GetRenderer()->GetMeshes().clear();
        UpdateExplorerView(true);
      }
      m_pRedrawTimer->start(20);
    }
  }
  return false;
}

bool MainWindow::LoadDataset(QStringList files, QString targetFilename,
                             bool bNoUserInteraction) {

  if(files.empty()) {
    T_ERROR("No files!");
    return false;
  }

  PleaseWaitDialog pleaseWait(this);
  pleaseWait.SetText("Loading dataset, please wait  ...");
  pleaseWait.AttachLabel(&m_MasterController);

  // First check to make sure the list of files we've been given makes sense.
  for(QStringList::const_iterator fn = files.begin();
      fn != files.end(); ++fn) {
    if(fn->isEmpty()) {
      T_ERROR("Empty filelist");
      return false;
    }
    if(!SysTools::FileExists(std::string(fn->toAscii()))) {
      QString strText = tr("File %1 not found.").arg(*fn);
      T_ERROR("%s", strText.toStdString().c_str());
      if(!bNoUserInteraction) {
        ShowCriticalDialog( "Load Error", strText);
      }
      return false;
    }
  }

  // now determine if we've been given a UVF, and can just open it and be done,
  // or if we need to convert the files.
  // If we were given multiple files, we don't need to do any work; we *know*
  // that needs to be converted.
  bool needs_conversion = true;
  if(files.size() == 1) {
    // check to see if we need to convert this file to a supported format.

    const IOManager& mgr = *(m_MasterController.IOMan());
    if(!mgr.NeedsConversion(files[0].toStdString())) {
      needs_conversion = false;

      // It might also be the case that the checksum is bad && we need to
      // report an error, but we don't bother with the checksum if they've
      // asked us not to in the preferences.
      if(!m_bQuickopen && false == mgr.Verify(files[0].toStdString())) {
        QString strText = tr("File %1 appears to be a broken UVF file: "
                             "the header looks ok, "
                             "but the checksum does not match.").arg(files[0]);
        T_ERROR("%s", strText.toStdString().c_str());
        if (!bNoUserInteraction) {
          ShowCriticalDialog("Load Error", strText);
        }
        return false;
      }
    }
  }

  QString filename = files[0];
  if(needs_conversion) {
    if (!bNoUserInteraction && targetFilename.isEmpty()) {
      targetFilename = GetConvFilename(files[0]);
    }
    if (targetFilename.isEmpty()) {
      T_ERROR("User interaction disabled but no targetFilename given");
      return false;
    }

    std::list<std::string> stdfiles;
    for(QStringList::const_iterator fn = files.begin();
        fn != files.end(); ++fn) {
      stdfiles.push_back(std::string(fn->toAscii()));
    }

    PleaseWaitDialog pleaseWait(this);
    pleaseWait.SetText("Converting, please wait  ...");
    pleaseWait.AttachLabel(&m_MasterController);

    if (!m_MasterController.IOMan()->ConvertDataset(
          stdfiles, std::string(targetFilename.toAscii()),
          m_strTempDir, bNoUserInteraction))
    {
      std::ostringstream error;
      error << "Unable to convert ";
      std::copy(stdfiles.begin(), stdfiles.end(),
                std::ostream_iterator<std::string>(error, ", "));
      error << " into " << std::string(targetFilename.toAscii());
      T_ERROR("%s", error.str().c_str());
      if (!bNoUserInteraction) {
        ShowCriticalDialog("Conversion Error", QString(error.str().c_str()));
      }

      pleaseWait.close();
      return false;
    } else {
      filename = targetFilename;
    }
    pleaseWait.close();
  }

  RenderWindow *renderWin = NULL;
  try {
    renderWin = CreateNewRenderWindow(filename);

    if(renderWin == NULL) {
      T_ERROR("Renderwindow creation failed.  Bailing...");
      return false;
    }

    renderWin->GetQtWidget()->show();  // calls RenderWindowActive automatically
    UpdateMenus();
    AddFileToMRUList(filename);
  } catch(tuvok::io::DSBricksOversized&) {
    WARNING("Bricks are too large.  Querying the user to see if we should "
            "rebrick the dataset.");
    if(renderWin) { delete renderWin; renderWin = NULL; }
    if(bNoUserInteraction) {
      T_ERROR("Dataset needs rebricking but ImageVis3D is not running interactively.");
      return false;
    }
    if(QMessageBox::Yes ==
       QMessageBox::question(NULL, "Rebricking required",
        "The bricking scheme in this dataset is not compatible with "
        "your current brick size settings.  Do you want to convert this "
        "dataset so that it can be loaded?  Note that this operation can "
        "take as long as originally converting the data took!",
        QMessageBox::Yes, QMessageBox::No)) {
      return RebrickDataset(filename, targetFilename, bNoUserInteraction);
    }
  }

  if (renderWin) RenderWindowActive(renderWin);
  if (CheckForMeshCapabilities(bNoUserInteraction, files)) return true;

  if(renderWin) {
    AbstrRenderer* ren = renderWin->GetRenderer();
    const Dataset& ds = ren->GetDataset();
    UINT64VECTOR3 dom_sz = ds.GetDomainSize(0);
    // Disable lighting for 2D datasets (images).
    if(dom_sz[2] == 1) {
      checkBox_Lighting->setChecked(false);
      checkBox_Lighting->setEnabled(false);
      SetLighting(false);
    }
  }

  return true;
}

bool MainWindow::RebrickDataset(QString filename, QString targetFilename,
                                bool bNoUserInteraction)
{
  QSettings settings;
  QString strLastDir = settings.value("Folders/GetConvFilename", ".").toString();

  QFileDialog::Options options;
#ifdef DETECTED_OS_APPLE
  options |= QFileDialog::DontUseNativeDialog;
#endif
  QString selectedFilter;
  QString rebrickedFilename;
  do {
    rebrickedFilename =
      QFileDialog::getSaveFileName(this, "Select filename for converted data",
                                   strLastDir, "Universal Volume Format (*.uvf)",
                                   &selectedFilter, options);
    if (!rebrickedFilename.isEmpty()) {
      rebrickedFilename = SysTools::CheckExt(
        std::string(rebrickedFilename.toAscii()), "uvf"
      ).c_str();

      if (rebrickedFilename == filename) {
        ShowCriticalDialog("Input Error",
                           "Rebricking can not be performed in place"
                           ", please select another file.");
      } else {
        settings.setValue("Folders/GetConvFilename",
                          QFileInfo(rebrickedFilename).absoluteDir().path());

        PleaseWaitDialog pleaseWait(this);
        pleaseWait.SetText("Rebricking, please wait  ...");
        pleaseWait.AttachLabel(&m_MasterController);

        if (!m_MasterController.IOMan()->ReBrickDataset(string(filename.toAscii()), string(rebrickedFilename.toAscii()), m_strTempDir)) {
          ShowCriticalDialog("Error during rebricking.",
                             "The system was unable to rebrick the data set, please check the error log for details (Menu -> \"Help\" -> \"Debug Window\").");
          return false;
        } else {
          pleaseWait.hide();
        }
      }
    } else {
      return false;
    }
  } while (rebrickedFilename == filename);

  return LoadDataset(QStringList(rebrickedFilename), targetFilename,
                     bNoUserInteraction);
}


void MainWindow::LoadDirectory() {
  QSettings settings;
  QString strLastDir = settings.value("Folders/LoadDirectory", ".").toString();

  QString directoryName =
    QFileDialog::getExistingDirectory(this, "Load Dataset from Directory",
                                      strLastDir);

  if (!directoryName.isEmpty()) {
    settings.setValue("Folders/LoadDirectory", directoryName);

    PleaseWaitDialog pleaseWait(this);
    pleaseWait.SetText("Scanning directory for files, please wait  ...");
    pleaseWait.AttachLabel(&m_MasterController);

    BrowseData browseDataDialog(m_MasterController, (QDialog*)&pleaseWait, directoryName, this);

    if (browseDataDialog.DataFound()) {
      if (browseDataDialog.exec() == QDialog::Accepted) {

        QFileDialog::Options options;
      #ifdef DETECTED_OS_APPLE
        options |= QFileDialog::DontUseNativeDialog;
      #endif
        QString selectedFilter;

        QString targetFilename = GetConvFilename(directoryName);
        if (targetFilename.isEmpty()) return;

        pleaseWait.SetText("Converting, please wait  ...");
        // label was detached when the dialog was closed by BrowseData
        pleaseWait.AttachLabel(&m_MasterController);

        /// @todo FIXME rewrite this ConvertDataset to take a shared_ptr
        /// instead of a raw pointer.
        if (!m_MasterController.IOMan()->ConvertDataset(
          &*browseDataDialog.GetStackInfo(), targetFilename.toStdString(),
          m_strTempDir)) {
          QString strText =
            tr("Unable to convert file stack from directory "
               "%1 into %2.").arg(directoryName).arg(targetFilename);
          ShowCriticalDialog("Conversion Error", strText);
          T_ERROR("%s", strText.toStdString().c_str());
        }
        pleaseWait.DetachLabel();

        RenderWindow *renderWin = CreateNewRenderWindow(targetFilename);

        if(NULL == renderWin || !renderWin->IsRenderSubsysOK()) {
          ShowCriticalDialog("Renderer Error",
                             "Unable to open the converted data set, "
                             "please check the error log for details "
                             "(Menu -> \"Help\" -> \"Debug Window\").");
          T_ERROR("Unable to open dataset, please check previous error msgs.");
          delete renderWin;
          return;
        }

        renderWin->GetQtWidget()->show();
        RenderWindowActive(renderWin);
        AddFileToMRUList(targetFilename);
      }
    } else {
      QString msg =
        tr("Error no valid files in directory %1 found.").arg(directoryName);
      ShowInformationDialog( tr("Problem"), msg);
    }
  }
}

bool MainWindow::ExportDataset(uint32_t iLODLevel, std::string targetFileName) {
  if (!m_pActiveRenderWin) {
    WARNING("No active render window");
    return false;
  }
  PleaseWaitDialog pleaseWait(this);
  pleaseWait.SetText("Exporting, please wait  ...");
  pleaseWait.AttachLabel(&m_MasterController);

  const UVFDataset *ds = dynamic_cast<UVFDataset*>(
    &(m_pActiveRenderWin->GetRenderer()->GetDataset())
  );
  bool bResult = m_MasterController.IOMan()->ExportDataset(
                            ds, iLODLevel, targetFileName,
                            m_strTempDir);

  pleaseWait.close();

  return bResult;
}

void MainWindow::ExportDataset() {
  if (!m_pActiveRenderWin) return;
  QFileDialog::Options options;
#ifdef DETECTED_OS_APPLE
  options |= QFileDialog::DontUseNativeDialog;
#endif
  QString selectedFilter;

  QSettings settings;
  QString strLastDir = settings.value("Folders/ExportDataset", ".").toString();

  QString dialogString = m_MasterController.IOMan()->GetExportDialogString().c_str();

  std::string ext;
  QString fileName;
  do {
    fileName = QFileDialog::getSaveFileName(this, "Export Current Dataset",
                                            strLastDir, dialogString,
                                            &selectedFilter, options);
    if(fileName.isEmpty()) {
      // not an error -- the user might get here if they wanted to cancel the
      // export.
      break;
    }
    // get it out of a QString && figure out what file type we're dealing with.
    std::string filter = std::string(selectedFilter.toAscii());
    size_t start = filter.find_last_of("*.")+1;
    size_t end = filter.find_last_of(")");
    ext = filter.substr(start, end-start);
    SysTools::TrimStr(ext);
    if(ext == "") {
      QMessageBox noformat;
      noformat.setText("No file extension: unknown export format");
      noformat.setIcon(QMessageBox::Critical);
      noformat.exec();
    }
  } while(ext == "");

  if (!fileName.isEmpty()) {
    settings.setValue("Folders/ExportDataset",
                      QFileInfo(fileName).absoluteDir().path());

    string strCompletefileName = SysTools::CheckExt(string(fileName.toAscii()),
                                                    ext);

    int iMaxLODLevel = int(m_pActiveRenderWin->GetRenderer()
                                             ->GetDataset()
                                              .GetLODLevelCount())-1;

    int iLODLevel = 0;
    if (iMaxLODLevel > 0) {
      int iMinLODLevel = 0;
      vector<QString> vDesc;
      for (int i = iMinLODLevel;i<=iMaxLODLevel;i++) {
        UINTVECTOR3 vLODSize = UINTVECTOR3(m_pActiveRenderWin->GetRenderer()
                                                             ->GetDataset()
                                                              .GetDomainSize(i));
        QString qstrDesc = tr("%1 x %2 x %3").arg(vLODSize.x).arg(vLODSize.y).arg(vLODSize.z);
        vDesc.push_back(qstrDesc);
      }
      LODDlg lodDlg("Which LOD Level do you want to export?", iMinLODLevel, iMaxLODLevel, vDesc, this);

      if (lodDlg.exec() != QDialog::Accepted)
        return;
      else
        iLODLevel = lodDlg.GetLOD();
    }

    PleaseWaitDialog pleaseWait(this);
    pleaseWait.SetText("Exporting, please wait  ...");
    pleaseWait.AttachLabel(&m_MasterController);

    if (!ExportDataset(iLODLevel, strCompletefileName)) {
      ShowCriticalDialog( "Error during dataset export.", "The system was unable to export the current data set, please check the error log for details (Menu -> \"Help\" -> \"Debug Window\").");
    } else {
      pleaseWait.hide();
      QString msg = tr("The dataset has been exported as %1.").arg(strCompletefileName.c_str());
      ShowInformationDialog( tr("Export successful"), msg);
    }
  }
}


bool MainWindow::ExportIsosurface(uint32_t iLODLevel, string targetFileName) {
    if (!m_pActiveRenderWin) {
      m_MasterController.DebugOut()->Warning("MainWindow::ExportIso", "No active renderwin");
      return false;
    }
    PleaseWaitDialog pleaseWait(this);
    pleaseWait.SetText("Exporting mesh, please wait  ...");
    pleaseWait.AttachLabel(&m_MasterController);

    int iValue = horizontalSlider_Isovalue->value();
    const UVFDataset *ds = dynamic_cast<UVFDataset*>(
      &(m_pActiveRenderWin->GetRenderer()->GetDataset())
    );

    FLOATVECTOR4 color(m_pActiveRenderWin->GetIsosufaceColor(),1.0f);


    bool bResult = m_MasterController.IOMan()->ExtractIsosurface(
                      ds, iLODLevel, iValue, color,
                      targetFileName, m_strTempDir
                   );
    pleaseWait.close();

    return bResult;
}

void MainWindow::ExportImageStack() {
  QFileDialog::Options options;
#ifdef DETECTED_OS_APPLE
  options |= QFileDialog::DontUseNativeDialog;
#endif
  QString selectedFilter;

  QSettings settings;
  QString strLastDir = settings.value("Folders/ExportImageStack", ".").toString();

  QString fileName =
    QFileDialog::getSaveFileName(this, "Export Current Dataset to a Set of Images",
         strLastDir,
         m_MasterController.IOMan()->GetImageExportDialogString().c_str(),
         &selectedFilter, options);

  if (!fileName.isEmpty()) {
    settings.setValue("Folders/ExportImageStack", QFileInfo(fileName).absoluteDir().path());
    string targetFileName = string(fileName.toAscii());

    int iMaxLODLevel = int(m_pActiveRenderWin->GetRenderer()->GetDataset().GetLODLevelCount())-1;

    int iLODLevel = 0;
    if (iMaxLODLevel > 0) {
      int iMinLODLevel = 0;
      vector<QString> vDesc;
      for (int i = iMinLODLevel;i<=iMaxLODLevel;i++) {
        UINTVECTOR3 vLODSize = UINTVECTOR3(m_pActiveRenderWin->GetRenderer()->GetDataset().GetDomainSize(i));
        QString qstrDesc = tr("%1 x %2 x %3").arg(vLODSize.x).arg(vLODSize.y).arg(vLODSize.z);
        vDesc.push_back(qstrDesc);
      }
      LODDlg lodDlg("For which LOD Level do you want to export the image stack?", iMinLODLevel, iMaxLODLevel, vDesc, this);

      if (lodDlg.exec() != QDialog::Accepted)
        return;
      else
        iLODLevel = lodDlg.GetLOD();
    }

    bool bAllDirs = QMessageBox::Yes == QMessageBox::question(this, "Texture Stack Exporter",
                                                                    "Do you want to export three stacks along all three directions? Otherwise only one stack along the z-axis is created.",
                                                              QMessageBox::Yes, QMessageBox::No);

    if(!ExportImageStack(uint32_t(iLODLevel), targetFileName, bAllDirs)) {
      ShowCriticalDialog( "Error during image stack export.", "The system was unable to export the current data set, please check the error log for details (Menu -> \"Help\" -> \"Debug Window\").");
      return;
    }

  }
}

bool MainWindow::ExportImageStack(uint32_t iLODLevel, std::string targetFileName, bool bAllDirs) {
    if (!m_pActiveRenderWin) {
      m_MasterController.DebugOut()->Warning("MainWindow::ExportImageStack", "No active renderwin");
      return false;
    }
    PleaseWaitDialog pleaseWait(this);
    pleaseWait.SetText("Exporting image stack, please wait  ...");
    pleaseWait.AttachLabel(&m_MasterController);

    const UVFDataset *ds = dynamic_cast<UVFDataset*>(
      &(m_pActiveRenderWin->GetRenderer()->GetDataset())
    );

    FLOATVECTOR4 color(m_pActiveRenderWin->GetIsosufaceColor(),1.0f);

    bool bResult = m_MasterController.IOMan()->ExtractImageStack( ds, m_1DTransferFunction->GetTrans(), iLODLevel,  targetFileName, m_strTempDir, bAllDirs);
    pleaseWait.close();

    return bResult;
}

void MainWindow::ExportIsosurface() {
  QFileDialog::Options options;
#ifdef DETECTED_OS_APPLE
  options |= QFileDialog::DontUseNativeDialog;
#endif
  QString selectedFilter;

  QSettings settings;
  QString strLastDir = settings.value("Folders/ExportIso", ".").toString();

  QString fileName =
    QFileDialog::getSaveFileName(this, "Export Current Isosurface to Mesh",
         strLastDir,
         m_MasterController.IOMan()->GetGeoExportDialogString().c_str(),
         &selectedFilter, options);

  if (!fileName.isEmpty()) {
    settings.setValue("Folders/ExportIso", QFileInfo(fileName).absoluteDir().path());
    string targetFileName = string(fileName.toAscii());

    // still a valid filename ext ?
    if (!m_MasterController.IOMan()->GetGeoConverterForExt(SysTools::ToLowerCase(SysTools::GetExt(string(fileName.toAscii()))),true,false)) {
      ShowCriticalDialog("Extension Error", "Unable to determine the file type from the file extension.");
      return;
    }

    int iMaxLODLevel = int(m_pActiveRenderWin->GetRenderer()->GetDataset().GetLODLevelCount())-1;

    int iLODLevel = 0;
    if (iMaxLODLevel > 0) {
      int iMinLODLevel = 0;
      vector<QString> vDesc;
      for (int i = iMinLODLevel;i<=iMaxLODLevel;i++) {
        UINTVECTOR3 vLODSize = UINTVECTOR3(m_pActiveRenderWin->GetRenderer()->GetDataset().GetDomainSize(i));
        QString qstrDesc = tr("%1 x %2 x %3").arg(vLODSize.x).arg(vLODSize.y).arg(vLODSize.z);
        vDesc.push_back(qstrDesc);
      }
      LODDlg lodDlg("For which LOD Level do you want to compute the mesh?", iMinLODLevel, iMaxLODLevel, vDesc, this);

      if (lodDlg.exec() != QDialog::Accepted)
        return;
      else
        iLODLevel = lodDlg.GetLOD();
    }

    if(!ExportIsosurface(uint32_t(iLODLevel), targetFileName)) {
      ShowCriticalDialog( "Error during mesh export.", "The system was unable to export the current data set, please check the error log for details (Menu -> \"Help\" -> \"Debug Window\").");
      return;
    }

    // if the choosen format supports import then ask the users if they want to re-import the mesh
    AbstrGeoConverter* c = m_MasterController.IOMan()->GetGeoConverterForExt(SysTools::GetExt(targetFileName),false,true);
    if (c) {
     if(QMessageBox::Yes ==
       QMessageBox::question(this, "Add Mesh to Project",
        "Do you want to integrate load the surface a part of this project?",
        QMessageBox::Yes, QMessageBox::No)) {

        AddGeometry(targetFileName);
      }
    }
  }
}

void MainWindow::CompareFiles(const std::string& strFile1, const std::string& strFile2) const {
  string strMessage = "";
  if (LargeRAWFile::Compare(strFile1, strFile2, &strMessage)) {
    m_MasterController.DebugOut()->Message("MainWindow::CompareFiles", "Files are identical!");
  } else {
    m_MasterController.DebugOut()->Warning("MainWindow::CompareFiles", "%s (Comparing %s %s)", strMessage.c_str(), strFile1.c_str(), strFile2.c_str());
  }
}


void MainWindow::MergeDatasets() {
  MergeDlg m(this);
  if (m.exec() == QDialog::Accepted) {
    vector <string> strFilenames;
    vector <double> vScales;
    vector<double>  vBiases;

    for (size_t i = 0;i<m.m_vDataSetList.size();i++) {
      strFilenames.push_back(m.m_vDataSetList[i]->m_strFilename);
      vScales.push_back(m.m_vDataSetList[i]->m_fScale);
      vBiases.push_back(m.m_vDataSetList[i]->m_fBias);
    }


    QFileDialog::Options options;
  #ifdef DETECTED_OS_APPLE
    options |= QFileDialog::DontUseNativeDialog;
  #endif
    QString selectedFilter;

    QSettings settings;
    QString strLastDir = settings.value("Folders/MergedOutput", ".").toString();

    QString dialogString = tr("%1%2").arg(m_MasterController.IOMan()->GetExportDialogString().c_str()).arg("Universal Volume Format (*.uvf);;");

    QString fileName =
      QFileDialog::getSaveFileName(this, "Merged Dataset",
           strLastDir,
           dialogString,&selectedFilter, options);

    if (!fileName.isEmpty()) {
      settings.setValue("Folders/MergedOutput", QFileInfo(fileName).absoluteDir().path());
      std::string stdFile = std::string(fileName.toAscii());
      if(SysTools::GetExt(stdFile) == "") {
        WARNING("no extension; assuming UVF.");
        stdFile = stdFile + ".uvf";
        // fix fileName too: used later if the user tries to load the data.
        fileName = QString(stdFile.c_str());
      }

      PleaseWaitDialog pleaseWait(this);
      pleaseWait.SetText("Merging ...");
      pleaseWait.AttachLabel(&m_MasterController);
      const IOManager& iom = *(m_MasterController.IOMan());
      if(!iom.MergeDatasets(strFilenames, vScales, vBiases, stdFile,
                            m_strTempDir, m.UseMax())) {
        ShowCriticalDialog("Data set Merge Error", "Unable to merge the selected data sets, make sure that the size and type of the data sets are the same.");
        return;
      }
      pleaseWait.close();
    }

    if (!m_bScriptMode) {
      if (QMessageBox::No == QMessageBox::question(NULL, "Dataset Merger", "Do you want to load the merged data set now?", QMessageBox::Yes, QMessageBox::No)) {
        return;
      }

      QString targetFilename = tr("%1%2").arg(fileName).arg(".uvf");
      LoadDataset(QStringList(fileName), targetFilename);
    }
  }
}

void MainWindow::SaveAspectRatioToUVF() {
  if (m_pActiveRenderWin) {
    m_pActiveRenderWin->GetRenderer()->SetDatasetIsInvalid(true);

    PleaseWaitDialog pleaseWait(this);
    pleaseWait.SetText("Writing rescale factors to file...");
    pleaseWait.AttachLabel(&m_MasterController);

    if (!m_pActiveRenderWin->GetRenderer()->GetDataset().SaveRescaleFactors()) {
      if (!m_bScriptMode) {
        QMessageBox::warning(this, "File Error", "Unable to save rescale factors to file.", QMessageBox::Ok);
      }
    }

    DOUBLEVECTOR3 vfRescaleFactors =  m_pActiveRenderWin->GetRenderer()->GetRescaleFactors();
    doubleSpinBox_RescaleX->setValue(vfRescaleFactors.x);
    doubleSpinBox_RescaleY->setValue(vfRescaleFactors.y);
    doubleSpinBox_RescaleZ->setValue(vfRescaleFactors.z);

    pleaseWait.close();
    m_pActiveRenderWin->GetRenderer()->SetDatasetIsInvalid(false);
  }
}


void MainWindow::CropData() {
  if (m_pActiveRenderWin) {

    // todo fix this by copying the meshes from the old dataset to the new one
    if (!m_pActiveRenderWin->GetRenderer()->GetDataset().GetMeshes().empty()) {
      QMessageBox::warning(this, "File Error", "Cropping datasets that contain meshes is not supported at the moment.", QMessageBox::Ok);
      return;
    }

    bool bKeepOldData = (QMessageBox::Yes == 
      QMessageBox::question(NULL, "Create Backup?", "Do you want to create a backup of the current dataset before cropping?", QMessageBox::Yes, QMessageBox::No));


    m_pActiveRenderWin->GetRenderer()->SetDatasetIsInvalid(true);

    PleaseWaitDialog pleaseWait(this);
    pleaseWait.SetText("Cropping dataset");
    pleaseWait.AttachLabel(&m_MasterController);

    ExtendedPlane p = m_pActiveRenderWin->GetRenderer()->GetClipPlane();
    FLOATMATRIX4 trans = m_pActiveRenderWin->GetFirst3DRegion()->rotation * 
                         m_pActiveRenderWin->GetFirst3DRegion()->translation;

    // get rid of the viewing transformation in the plane
    p.Transform(trans.inverse(),false);

    if (!m_pActiveRenderWin->GetRenderer()->CropDataset(m_strTempDir,bKeepOldData)) {
      if (!m_bScriptMode) {
        QMessageBox::warning(this, "File Error", "Unable to crop dataset, is the file write protected?", QMessageBox::Ok);
      }
    } else {
      ToggleClipPlane(false);

    }
    m_pActiveRenderWin->GetRenderer()->SetDatasetIsInvalid(false);

    RenderWindow* current = m_pActiveRenderWin;
    m_pActiveRenderWin = NULL; // set m_pActiveRenderWin to NULL so RenderWindowActive thinks it has changed
    RenderWindowActive(current);

    pleaseWait.close();
  }
}
