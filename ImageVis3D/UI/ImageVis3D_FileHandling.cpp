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


//!    File   : ImageVis3D_FileHandling.cpp
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
#include <QtWidgets/QFileDialog>
#include <QtWidgets/QMessageBox>

#include "Basics/Mesh.h"
#include "Renderer/RenderMesh.h" // we only need this include for proper down cast from RenderMesh to Mesh
#include "Basics/SysTools.h"
#include "Controller/Controller.h"
#include "IO/TuvokIOError.h"
#include "IO/uvfDataset.h"

#include "DebugOut/QTLabelOut.h"
#include "LODDlg.h"
#include "MergeDlg.h"
#include "PleaseWait.h"

#include "LuaScripting/TuvokSpecific/LuaDatasetProxy.h"
#include "LuaScripting/TuvokSpecific/LuaTuvokTypes.h"
#include "LuaScripting/TuvokSpecific/LuaTransferFun1DProxy.h"

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

  shared_ptr<LuaScripting> ss(m_MasterController.LuaScript());
  QString dialogString = QString::fromStdWString(
      ss->cexecRet<std::wstring>("tuvok.io.getLoadDialogString"));

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
    const char* rwe ="Could not open a render window!  This normally "
                     "means ImageVis3D does not support your GPU.  Please"
                     " check the debug log ('Help | Debug Window') for "
                     "errors, and/or use 'Help | Report an Issue' to "
                     "notify the ImageVis3D developers.";
    try {
      if(!LoadDataset(files)) {
        if(m_bIgnoreLoadDatasetFailure == false) {
          ShowCriticalDialog("Render window initialization failed.", rwe);
        }
        m_bIgnoreLoadDatasetFailure = false;
      }
    } catch(const std::exception& e) {
      if(e.what() != NULL) {
        if (strlen(e.what()) > 0)
          ShowCriticalDialog("Could not load data set!", e.what());
        else
          ShowCriticalDialog("Could not load data set!", "Operation has been canceled.");
      } else {
        ShowCriticalDialog("Render window initialization failed.", rwe);
      }
    }
  }
}


void MainWindow::ExportGeometry() {
 QFileDialog::Options options;
#ifdef DETECTED_OS_APPLE
  options |= QFileDialog::DontUseNativeDialog;
#endif
  QString selectedFilter("g3d");

  QSettings settings;
  QString strLastDir = settings.value("Folders/ExportMesh", ".").toString();

  shared_ptr<LuaScripting> ss(m_MasterController.LuaScript());
  QString fileName =
    QFileDialog::getSaveFileName(this, "Export Current Mesh to File",
         strLastDir,
         QString::fromStdWString(ss->cexecRet<std::wstring>("tuvok.io.getGeoExportDialogString")),
         &selectedFilter, options);

  if (!fileName.isEmpty()) {
    settings.setValue("Folders/ExportMesh", QFileInfo(fileName).absoluteDir().path());
    wstring targetFileName = fileName.toStdWString();

    // still a valid filename ext ?
    if (ss->cexecRet<bool>("tuvok.io.hasGeoConverterForExt", 
                           SysTools::ToLowerCase(SysTools::GetExt(fileName.toStdWString())),
                           true, false) == false) {
      ShowCriticalDialog("Extension Error", 
                         "Unable to determine the file type "
                         "from the file extension.");
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

bool MainWindow::ExportGeometry(size_t i, const std::wstring& strFilename) {
  if (!m_pActiveRenderWin) return false;
  LuaClassInstance ds = m_pActiveRenderWin->GetRendererDataset();
  shared_ptr<LuaScripting> ss(m_MasterController.LuaScript());
  if (!ds.isValid(ss) || 
      (ss->cexecRet<LuaDatasetProxy::DatasetType>(ds.fqName() + ".getDSType") !=
       LuaDatasetProxy::UVF) )
    return false;

  PleaseWaitDialog pleaseWait(this);
  pleaseWait.SetText("Exporting Mesh...");
  pleaseWait.AttachLabel(&m_MasterController);

#if 0
  // Old version:
  // Export the mesh as stored on disk.
  std::vector<shared_ptr<Mesh>> meshes =
      ss->cexecRet<std::vector<shared_ptr<Mesh>>>(ds.fqName() + ".getMeshes");
  shared_ptr<Mesh> mesh = meshes[i];
#else
  // Changed by Alex:
  // Export the visible rendered mesh with updated colors as we see it.
  // The export will bake the colors into the exported mesh anyway.
  // The index i should be identical to the disk mesh list.
  std::vector<shared_ptr<RenderMesh>> rmeshes = m_pActiveRenderWin->GetRendererMeshes();
  shared_ptr<RenderMesh> rmesh = rmeshes[i];
  shared_ptr<Mesh> mesh = rmesh;
#endif

  return ss->cexecRet<bool>("tuvok.io.exportMesh", mesh, strFilename);
}

void MainWindow::RemoveGeometry() {
  if (!m_pActiveRenderWin) return;
  LuaClassInstance ds = m_pActiveRenderWin->GetRendererDataset();
  shared_ptr<LuaScripting> ss(m_MasterController.LuaScript());
  if (!ds.isValid(ss) || 
      (ss->cexecRet<LuaDatasetProxy::DatasetType>(ds.fqName() + ".getDSType") !=
       LuaDatasetProxy::UVF) )
    return;

  PleaseWaitDialog pleaseWait(this);
  pleaseWait.SetText("Removing Mesh from UVF file...");
  pleaseWait.AttachLabel(&m_MasterController);

  int iCurrent = listWidget_DatasetComponents->currentRow();

  if (iCurrent < 1 || iCurrent >= listWidget_DatasetComponents->count()) {
    T_ERROR("No Mesh selected.");
    return;
  }

  m_pActiveRenderWin->SetDatasetIsInvalid(true);

  if (!ss->cexecRet<bool>(ds.fqName() + ".removeMesh", size_t(iCurrent-1))) {
    pleaseWait.close();
    ShowCriticalDialog("Mesh Removal Failed.",
             "Could not remove mesh from the UVF file, "
             "maybe the file is write protected? For details please "
             "check the debug log ('Help | Debug Window').");
  } else {
    m_pActiveRenderWin->RemoveMeshData(size_t(iCurrent-1));
    UpdateExplorerView(true);
    pleaseWait.close();
  }

  m_pActiveRenderWin->SetDatasetIsInvalid(false);
}

void MainWindow::AddGeometry(const std::wstring& filename) {
  if (!m_pActiveRenderWin) return;

  if(!m_pActiveRenderWin->SupportsMeshes()) {
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
        LoadDataset(dataset.toStdWString());
        m_eVolumeRendererType = currentType;
      }
  }


  shared_ptr<LuaScripting> ss = m_MasterController.LuaScript();
  LuaClassInstance ds = m_pActiveRenderWin->GetRendererDataset();
  LuaDatasetProxy::DatasetType type = 
      ss->cexecRet<LuaDatasetProxy::DatasetType>(ds.fqName() + ".getDSType");
  if (type != LuaDatasetProxy::UVF) {
    ShowCriticalDialog("Mesh Import Failed.",
                 "Mesh Integration only supported for UVF datasets.");
    return;
  }

  PleaseWaitDialog pleaseWait(this);

  pleaseWait.SetText("Loading mesh, please wait  ...");
  pleaseWait.AttachLabel(&m_MasterController);

  std::shared_ptr<Mesh> m;
  try {
    m = ss->cexecRet<std::shared_ptr<Mesh>>("tuvok.io.loadMesh", filename);
  } catch (const tuvok::io::DSOpenFailed& err) {
    WARNING("Conversion failed! %s", err.what());
  }

  if (!m)  {
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

  m_pActiveRenderWin->SetDatasetIsInvalid(true);

  shared_ptr<const Mesh> constMeshPtr = m;
  if (!ss->cexecRet<bool>(ds.fqName() + ".appendMesh", constMeshPtr)) {
    pleaseWait.close();
    ShowCriticalDialog("Mesh Import Failed.",
                 "Could not integrate the mesh into this UVF file. "
                 "For details please check the debug log "
                 "('Help | Debug Window').");
  } else {
    pleaseWait.SetText("Creating render resources, please wait  ...");
    m_pActiveRenderWin->ScanForNewMeshes();
    pleaseWait.close();
    UpdateExplorerView(true);
  }

  m_pActiveRenderWin->SetDatasetIsInvalid(false);
}

void MainWindow::AddGeometry() {
  QFileDialog::Options options;
#ifdef DETECTED_OS_APPLE
  options |= QFileDialog::DontUseNativeDialog;
#endif

  QSettings settings;
  QString selFilter;

  shared_ptr<LuaScripting> ss(m_MasterController.LuaScript());
  QString dialogString = QString::fromStdWString(
      ss->cexecRet<std::wstring>("tuvok.io.getLoadGeoDialogString"));

  QString strLastDir = settings.value("Folders/AddTriGeo", ".").toString();

  QString geoFile =
    QFileDialog::getOpenFileName(this, "Select Geometry File", strLastDir,
                                  dialogString,
                                  &selFilter, options);
  if(geoFile.isEmpty()) {
    return;
  }

  settings.setValue("Folders/AddTriGeo", QFileInfo(geoFile).absoluteDir().path());

  AddGeometry(geoFile.toStdWString());
}

void MainWindow::LoadDataset(const std::wstring& strFilename) {
  try {
    QStringList files;
    files.append(QString::fromStdWString(strFilename));
    if(!LoadDataset(files) != 0) {
      if(m_bIgnoreLoadDatasetFailure == false) {
        ShowCriticalDialog("Render window initialization failed.",
          "Could not open a render window!  This normally "
          "means ImageVis3D does not support your GPU.  Please"
          " check the debug log ('Help | Debug Window') for "
          "errors, and/or use 'Help | Report an Issue' to "
          "notify the ImageVis3D developers."
        );
      }
      m_bIgnoreLoadDatasetFailure = false;
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
                                SysTools::GetFilename(string(sourceName.toStdString())),
                                                      "uvf"
                              ).c_str();
  
  strLastDir = tr("%1/%2").arg(strLastDir).arg(suggestedFileName);

  QString targetFilename =
    QFileDialog::getSaveFileName(this, "Select filename for converted data",
                                 strLastDir, "Universal Volume Format (*.uvf)",
                                 &selectedFilter, options);

  if (!targetFilename.isEmpty()) {
    targetFilename = QString::fromStdWString(SysTools::CheckExt(targetFilename.toStdWString(), L"uvf"));
    settings.setValue("Folders/GetConvFilename",
                      QFileInfo(targetFilename).absoluteDir().path());
  }

  return targetFilename;
}


bool MainWindow::LoadDataset(const std::vector< std::wstring >& strParams) {
  if (strParams.size() < 1 || strParams.size() > 2) {
    return false;
  }
  wstring inFile = strParams[0], convFile;
  if (strParams.size() == 1)  {
    convFile = SysTools::ChangeExt(inFile, L"uvf");
  } else {
    convFile = strParams[1];
  }
  QStringList l;
  l.append(QString::fromStdWString(convFile));
  return LoadDataset(l, QString::fromStdWString(inFile), false);
}

bool MainWindow::CheckForMeshCapabilities(bool bNoUserInteraction,
                                          QStringList files) {
  if (bNoUserInteraction) {
    if (m_pActiveRenderWin && 
      !m_pActiveRenderWin->SupportsMeshes() &&
      m_pActiveRenderWin->GetRendererMeshes().size() > 0) {
        WARNING("This dataset contains mesh data but the current "
                "renderer does not supports rendering meshes. Mesh "
                "rendering is disabled until you switch to a renderer "
                "that supports this feature e.g. the 3D slice "
                "based volume renderer.");
        m_pActiveRenderWin->ClearRendererMeshes();
        UpdateExplorerView(true);
    }
  } else {
    if (m_pActiveRenderWin && 
      !m_pActiveRenderWin->SupportsMeshes() &&
      m_pActiveRenderWin->GetRendererMeshes().size() > 0) {
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
        m_pRedrawTimer->start(IV3D_TIMER_INTERVAL);
        return true;
      } else {
        m_pActiveRenderWin->ClearRendererMeshes();
        UpdateExplorerView(true);
      }
      m_pRedrawTimer->start(IV3D_TIMER_INTERVAL);
    }
  }
  return false;
}

bool MainWindow::LoadDataset(QStringList files, QString targetFilename,
                             bool bNoUserInteraction) {
  std::vector<std::wstring> stdFiles;
  for (QStringList::iterator it = files.begin(); it != files.end();
      ++it)
  {
    wstring datasetName = it->toStdWString();
    stdFiles.push_back(datasetName);
  }

  wstring stdTargetFilename = targetFilename.toStdWString();
  try {
    LuaClassInstance inst =
      m_MasterController.LuaScript()->cexecRet<LuaClassInstance>(
        "iv3d.rendererWithParams.new", stdFiles, stdTargetFilename,
        bNoUserInteraction);

    return !inst.isDefaultInstance();
  } catch (const std::exception & e) {
    if (!bNoUserInteraction) {
      ShowCriticalDialog("Unable to load file",e.what());
    }
    return false;

  }
}

RenderWindow* MainWindow::LuaLoadDatasetInternal(const std::vector<wstring>& stdFiles,
                                                 const std::wstring& stdTargetFilename,
                                                 bool bNoUserInteraction)
{
  QStringList files;
  for (auto it = stdFiles.begin(); it != stdFiles.end();
      ++it)
  {
    files.push_back(QString::fromStdWString(*it));
  }

  QString targetFilename = QString::fromStdWString(stdTargetFilename);

  bool retVal = LoadDatasetInternal(files, targetFilename, bNoUserInteraction);
  if (retVal == true)
  {
    return m_pLastLoadedRenderWin;
  }
  else
  {
    m_MasterController.LuaScript()->vPrint("False returned from load dataset "
        "internal.");
    return NULL;
  }
}

bool MainWindow::LoadDatasetInternal(QStringList files, QString targetFilename,
                                     bool bNoUserInteraction)
{

  if(files.empty()) {
    T_ERROR("No files!");
    return false;
  }

  PleaseWaitDialog pleaseWaitLoading(this);
  pleaseWaitLoading.SetText("Loading dataset, please wait  ...");
  pleaseWaitLoading.AttachLabel(&m_MasterController);

  // First check to make sure the list of files we've been given makes sense.
  for(QStringList::const_iterator fn = files.begin();
      fn != files.end(); ++fn) {
    if(fn->isEmpty()) {
      T_ERROR("Empty filelist");
      return false;
    }
    if(!SysTools::FileExists(fn->toStdWString())) {
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
  shared_ptr<LuaScripting> ss(m_MasterController.LuaScript());
  bool needs_conversion = true;
  if(files.size() == 1) {
    // check to see if we need to convert this file to a supported format.

    if(!ss->cexecRet<bool>("tuvok.io.needsConversion",files[0].toStdWString())) {
      needs_conversion = false;

      // It might also be the case that the checksum is bad && we need to
      // report an error, but we don't bother with the checksum if they've
      // asked us not to in the preferences.
      if(!m_bQuickopen && 
         false == ss->cexecRet<bool>("tuvok.io.verify", files[0].toStdWString()))
      {
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

    std::list<std::wstring> stdfiles;
    for(QStringList::const_iterator fn = files.begin();
        fn != files.end(); ++fn) {
      stdfiles.push_back(fn->toStdWString());
    }

    PleaseWaitDialog pleaseWaitConverting(this);
    pleaseWaitConverting.SetText("Converting, please wait  ...");
    pleaseWaitConverting.AttachLabel(&m_MasterController);

    try {
      ss->cexec("tuvok.io.convertDataset", stdfiles,
                targetFilename.toStdWString(), m_strTempDir, 
                bNoUserInteraction, false);
      filename = targetFilename;
    } catch(const tuvok::io::IOException& e) {
      // create a (hopefully) useful error message
      std::ostringstream error;
      error << "Unable to convert ";
      for (std::wstring s : stdfiles) {
          error << SysTools::toNarrow(s) << " ";
      }
      error << " into " << targetFilename.toStdString()
            << ": " << e.what();
      T_ERROR("%s", error.str().c_str());
      if(!bNoUserInteraction) {
        ShowCriticalDialog("Conversion Error", QString(error.str().c_str()));
      }

      // now close that dialog and bail.
      pleaseWaitConverting.close();
      return false;
    }

    pleaseWaitConverting.close();
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
      // m_bIgnoreLoadDatasetFailure is an ugly hack, but I can't see any other
      // *good* options.
      // 
      // An alternative to m_bIgnoreLoadDatasetFailure is to throw a custom 
      // exception which would include bNoUserInteraction's value, and catch  
      // the exception in MainWindow::LoadDataset(std::string strFilename). 
      // (and MainWindow::LoadDataset(), and MainWindow::OpenRecentFile).
      // We would perform the rebricking upon catching the exception.
      // This method seemed equally hacky so I opted for the smallest code 
      // footprint and inserted this boolean flag.
      //
      // Also, we can use the flag when the user specifies 'no' on the dialog.
      // It doesn't make much sense to tell the user that we failed to
      // load the RenderWindow because they told us to not load it =P.
      //
      // The problem: if we reach here, we were in the process of creating
      // a series of Lua classes to support a render window. Since we are no
      // longer creating a viable render window (renderWin was deleted, and 
      // RebrickDataset is creating a new render window), lua needs to be 
      // notified and clean up any supporting classes.
      //
      // The only way to notify Lua is to do 1 of 2 things:
      // 1) Return false from this function (behaving as if the requested 
      //    dataset failed to load -- which it did).
      // 2) Or throw an exception and let lua capture and rethrow it.
      //
      // I opted for #1.
      RebrickDataset(filename, targetFilename, bNoUserInteraction);
      m_bIgnoreLoadDatasetFailure = true;
      return false;
    } else {
      m_bIgnoreLoadDatasetFailure = true;
      return false;
    }
  }

  if (renderWin) RenderWindowActive(renderWin);
  if (CheckForMeshCapabilities(bNoUserInteraction, files)) return true;

  if(renderWin) {
    LuaClassInstance ds = renderWin->GetRendererDataset();
    shared_ptr<LuaScripting> _ss = m_MasterController.LuaScript();
    UINT64VECTOR3 dom_sz = _ss->cexecRet<UINT64VECTOR3>(ds.fqName() + 
                                                       ".getDomainSize", 
                                                       size_t(0), 
                                                       size_t(0));
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

  shared_ptr<LuaScripting> ss(m_MasterController.LuaScript());
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
      rebrickedFilename = QString::fromStdWString(SysTools::CheckExt(
        rebrickedFilename.toStdWString(), L"uvf"));

      if (rebrickedFilename == filename) {
        ShowCriticalDialog("Input Error",
                           "Rebricking can not be performed in place, "
                           "please select another file.");
      } else {
        settings.setValue("Folders/GetConvFilename",
                          QFileInfo(rebrickedFilename).absoluteDir().path());

        PleaseWaitDialog pleaseWait(this);
        pleaseWait.SetText("Rebricking, please wait  ...");
        pleaseWait.AttachLabel(&m_MasterController);

        if(!ss->cexecRet<bool>("tuvok.io.rebrickDataset",
                               filename.toStdWString(),
                               rebrickedFilename.toStdWString(),
                               m_strTempDir)) {
          ShowCriticalDialog("Error during rebricking.",
                             "The system was unable to rebrick the data set, "
                             "please check the error log for details (Menu -> "
                             "\"Help\" -> \"Debug Window\").");
          return false;
        } else {
          pleaseWait.hide();
        }
      }
    } else {
      return false;
    }
  } while (rebrickedFilename == filename);
  
  QStringList l;
  l.append(rebrickedFilename);
  return LoadDataset(l, targetFilename, bNoUserInteraction);
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

        shared_ptr<LuaScripting> ss(m_MasterController.LuaScript());
        if (!ss->cexecRet<bool>("tuvok.io.convertDatasetWithStack",
                                browseDataDialog.GetStackInfo(),
                                targetFilename.toStdWString(),
                                m_strTempDir, false)) {
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

bool MainWindow::ExportDataset(uint32_t iLODLevel, const std::wstring& targetFileName) {
  if (!m_pActiveRenderWin) {
    WARNING("No active render window");
    return false;
  }
  PleaseWaitDialog pleaseWait(this);
  pleaseWait.SetText("Exporting, please wait  ...");
  pleaseWait.AttachLabel(&m_MasterController);

  shared_ptr<LuaScripting> ss = m_MasterController.LuaScript();
  bool bResult = ss->cexecRet<bool>("tuvok.io.exportDataset",
                                    m_pActiveRenderWin->GetRendererDataset(), 
                                    static_cast<uint64_t>(iLODLevel), 
                                    targetFileName,
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

  shared_ptr<LuaScripting> ss(m_MasterController.LuaScript());
  QString dialogString = QString::fromStdWString(
      ss->cexecRet<std::wstring>("tuvok.io.getExportDialogString"));

  std::wstring ext;
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
    ext = SysTools::GetExt(selectedFilter.toStdWString());
    ext = ext.substr(0, ext.length() - 1);

    if(ext == L"") {
      QMessageBox noformat;
      noformat.setText("No file extension: unknown export format");
      noformat.setIcon(QMessageBox::Critical);
      noformat.exec();
    }
  } while(ext == L"");

  if (!fileName.isEmpty()) {
    settings.setValue("Folders/ExportDataset",
                      QFileInfo(fileName).absoluteDir().path());

    std::wstring strCompletefileName = SysTools::CheckExt(fileName.toStdWString(),ext);

    shared_ptr<LuaScripting> _ss = m_MasterController.LuaScript(); 
    LuaClassInstance ds = m_pActiveRenderWin->GetRendererDataset();
    int iMaxLODLevel = static_cast<int>(
        _ss->cexecRet<uint64_t>(ds.fqName() + ".getLODLevelCount")) - 1;

    int iLODLevel = 0;
    if (iMaxLODLevel > 0) {
      int iMinLODLevel = 0;
      vector<QString> vDesc;
      for (int i = iMinLODLevel;i<=iMaxLODLevel;i++) {
        UINTVECTOR3 vLODSize = UINTVECTOR3(_ss->cexecRet<UINT64VECTOR3>(
                ds.fqName() + ".getDomainSize", size_t(i), size_t(0)));
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
      QString msg = tr("The dataset has been exported as %1.").arg(QString::fromStdWString(strCompletefileName));
      ShowInformationDialog( tr("Export successful"), msg);
    }
  }
}


bool MainWindow::ExportIsosurface(uint32_t iLODLevel, const std::wstring& targetFileName) {
    if (!m_pActiveRenderWin) {
      m_MasterController.DebugOut()->Warning("MainWindow::ExportIso", "No active renderwin");
      return false;
    }
    PleaseWaitDialog pleaseWait(this);
    pleaseWait.SetText("Exporting mesh, please wait  ...");
    pleaseWait.AttachLabel(&m_MasterController);

    int iValue = horizontalSlider_Isovalue->value();

    FLOATVECTOR4 color(m_pActiveRenderWin->GetIsosurfaceColor(),1.0f);

    shared_ptr<LuaScripting> ss = m_MasterController.LuaScript();
    bool bResult = ss->cexecRet<bool>("tuvok.io.extractIsosurface",
                                      m_pActiveRenderWin->GetRendererDataset(), 
                                      static_cast<uint64_t>(iLODLevel), 
                                      static_cast<double>(iValue), color,
                                      targetFileName, m_strTempDir);
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

  shared_ptr<LuaScripting> ss(m_MasterController.LuaScript());
  const std::wstring filterStr = ss->cexecRet<std::wstring>("tuvok.io.getImageExportDialogString");

  QString fileName =
    QFileDialog::getSaveFileName(this, "Export Current Dataset to a Set of Images",
         strLastDir, QString::fromStdWString(filterStr),&selectedFilter, options);

  if (!fileName.isEmpty()) {
    settings.setValue("Folders/ExportImageStack", QFileInfo(fileName).absoluteDir().path());
    std::wstring targetFileName = fileName.toStdWString();
    
    const std::wstring selectedFilterStr = selectedFilter.toStdWString();
    const std::wstring filterExt = ss->cexecRet<std::wstring>("tuvok.io.imageExportDialogFilterToExt", selectedFilterStr);

    if (SysTools::GetExt(targetFileName).empty())  {
      if (filterExt.empty()) {
        ShowCriticalDialog( "Error during image stack export.", "Unable to determine file type from filename.");
        return;
      } else {
        targetFileName += std::wstring(L".") + filterExt;
      }
    }

    shared_ptr<LuaScripting> _ss = m_MasterController.LuaScript();
    LuaClassInstance ds = m_pActiveRenderWin->GetRendererDataset();
    int iMaxLODLevel = static_cast<int>(
        _ss->cexecRet<uint64_t>(ds.fqName() + ".getLODLevelCount") ) - 1;

    int iLODLevel = 0;
    if (iMaxLODLevel > 0) {
      int iMinLODLevel = 0;
      vector<QString> vDesc;
      for (int i = iMinLODLevel;i<=iMaxLODLevel;i++) {
        UINTVECTOR3 vLODSize = UINTVECTOR3( _ss->cexecRet<UINT64VECTOR3>(
                ds.fqName() + ".getDomainSize", size_t(i), size_t(0)) );
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

bool MainWindow::ExportImageStack(uint32_t iLODLevel, const std::wstring& targetFileName, bool bAllDirs) {
    if (!m_pActiveRenderWin) {
      m_MasterController.DebugOut()->Warning("MainWindow::ExportImageStack", "No active renderwin");
      return false;
    }
    PleaseWaitDialog pleaseWait(this);
    pleaseWait.SetText("Exporting image stack, please wait  ...");
    pleaseWait.AttachLabel(&m_MasterController);

    FLOATVECTOR4 color(m_pActiveRenderWin->GetIsosurfaceColor(),1.0f);

    shared_ptr<LuaScripting> ss = m_MasterController.LuaScript();
    LuaClassInstance tf = m_1DTransferFunction->GetTrans();
    bool bResult = ss->cexecRet<bool>("tuvok.io.extractImageStack", 
                                      m_pActiveRenderWin->GetRendererDataset(),
                                      tf, static_cast<uint64_t>(iLODLevel), 
                                      targetFileName, m_strTempDir, bAllDirs);
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

  shared_ptr<LuaScripting> ss(m_MasterController.LuaScript());
  QString fileName =
    QFileDialog::getSaveFileName(this, "Export Current Isosurface to Mesh",
         strLastDir,
         QString::fromStdWString(ss->cexecRet<wstring>("tuvok.io.getGeoExportDialogString")),
         &selectedFilter, options);

  if (!fileName.isEmpty()) {
    settings.setValue("Folders/ExportIso", QFileInfo(fileName).absoluteDir().path());
    const std::wstring targetFileName = fileName.toStdWString();

    // still a valid filename ext ?
    if (!ss->cexecRet<bool>("tuvok.io.hasGeoConverterForExt", 
                            SysTools::ToLowerCase(SysTools::GetExt(
                                    fileName.toStdWString())),
                            true,false)) {
      ShowCriticalDialog("Extension Error", 
                         "Unable to determine the file type "
                         "from the file extension.");
      return;
    }

    LuaClassInstance ds = m_pActiveRenderWin->GetRendererDataset();
    int iMaxLODLevel = int(ss->cexecRet<uint64_t>(ds.fqName() + ".getLODLevelCount"))-1;

    int iLODLevel = 0;
    if (iMaxLODLevel > 0) {
      int iMinLODLevel = 0;
      vector<QString> vDesc;
      for (int i = iMinLODLevel;i<=iMaxLODLevel;i++) {
        UINTVECTOR3 vLODSize = UINTVECTOR3(
            ss->cexecRet<UINT64VECTOR3>(ds.fqName() + ".getDomainSize",
                                        static_cast<size_t>(i),
                                        static_cast<size_t>(0)));
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
    bool hasConverter = ss->cexecRet<bool>("tuvok.io.hasGeoConverterForExt",
                                           SysTools::GetExt(targetFileName),
                                           false,true);
    if (hasConverter) {
     if(QMessageBox::Yes ==
       QMessageBox::question(this, "Add Mesh to Project",
        "Do you want to load the surface a part of this project?",
        QMessageBox::Yes, QMessageBox::No)) {

        AddGeometry(targetFileName);
      }
    }
  }
}

void MainWindow::CompareFiles(const std::wstring& strFile1, const std::wstring& strFile2) const {
  std::wstring strMessage = L"";
  if (LargeRAWFile::Compare(strFile1, strFile2, &strMessage)) {
    m_MasterController.DebugOut()->Message("MainWindow::CompareFiles", "Files are identical!");
  } else {
    m_MasterController.DebugOut()->Warning("MainWindow::CompareFiles", "%s (Comparing %s %s)", 
      SysTools::toNarrow(strMessage).c_str(), 
      SysTools::toNarrow(strFile1).c_str(), 
      SysTools::toNarrow(strFile2).c_str());
  }
}


void MainWindow::MergeDatasets() {
  MergeDlg m(this);
  if (m.exec() == QDialog::Accepted) {
    vector <wstring> strFilenames;
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

    shared_ptr<LuaScripting> ss(m_MasterController.LuaScript());
    QString dialogString = tr("%1%2")
        .arg(QString::fromStdWString(ss->cexecRet<wstring>("tuvok.io.getExportDialogString")))
        .arg("Universal Volume Format (*.uvf);;");

    QString fileName =
      QFileDialog::getSaveFileName(this, "Merged Dataset",
           strLastDir,
           dialogString,&selectedFilter, options);

    if (!fileName.isEmpty()) {
      settings.setValue("Folders/MergedOutput", QFileInfo(fileName).absoluteDir().path());
      std::wstring stdFile = fileName.toStdWString();
      if(SysTools::GetExt(stdFile) == L"") {
        WARNING("no extension; assuming UVF.");
        stdFile = stdFile + L".uvf";
        // fix fileName too: used later if the user tries to load the data.
        fileName = QString::fromStdWString(stdFile);
      }

      PleaseWaitDialog pleaseWait(this);
      pleaseWait.SetText("Merging ...");
      pleaseWait.AttachLabel(&m_MasterController);
      if (m.UseCustomExpr()) {
        try {
          ss->cexec("tuvok.io.evaluateExpression", 
                    m.GetCustomExpr().c_str(),
                    strFilenames, stdFile);
        }
        catch (tuvok::Exception& e) {
          std::string errMsg = "Unable to merge the selected data sets, make "
                               "sure that the size and type of the data sets "
                               "are the same. Also, check your expression.";
          if (strlen(e.what()) > 0) {
            errMsg += "  Error: ";
            errMsg += e.what();
          }
          ShowCriticalDialog("Data Set Expression Merge Error", errMsg.c_str());
          return;
        }
      } else {
        if(!ss->cexecRet<bool>("tuvok.io.mergeDatasets",
                               strFilenames, vScales, vBiases, stdFile,
                               m_strTempDir, m.UseMax())) {
          ShowCriticalDialog("Data set Merge Error",
                             "Unable to merge the selected data sets, make "
                             "sure that the size and type of the data sets "
                             "are the same.");
          return;
        }
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
    m_pActiveRenderWin->SetDatasetIsInvalid(true);

    PleaseWaitDialog pleaseWait(this);
    pleaseWait.SetText("Writing rescale factors to file...");
    pleaseWait.AttachLabel(&m_MasterController);

    shared_ptr<LuaScripting> ss(m_MasterController.LuaScript());
    LuaClassInstance ds = m_pActiveRenderWin->GetRendererDataset();

    try {
      if (!ss->cexecRet<bool>(ds.fqName() + ".saveRescaleFactors")) {
        if (!m_bScriptMode) {
          QMessageBox::warning(this, "File Error", "Unable to save rescale factors to file.", QMessageBox::Ok);
        }
      }
    }
    catch (std::exception& e) {
      std::string errMsg = "Unable to save scale factors to UVF file.";
      if (strlen(e.what()) > 0) {
        errMsg += "  Error: ";
        errMsg += e.what();
      }
      QMessageBox::warning(this, "Scale Factor Save Error", errMsg.c_str(), 
                           QMessageBox::Ok);
    }

    DOUBLEVECTOR3 vfRescaleFactors = 
        ss->cexecRet<DOUBLEVECTOR3>(ds.fqName() + ".getRescaleFactors");
    doubleSpinBox_RescaleX->setValue(vfRescaleFactors.x);
    doubleSpinBox_RescaleY->setValue(vfRescaleFactors.y);
    doubleSpinBox_RescaleZ->setValue(vfRescaleFactors.z);

    pleaseWait.close();
    m_pActiveRenderWin->SetDatasetIsInvalid(false);
  }
}


void MainWindow::CropData() {
  if (m_pActiveRenderWin) {

    // todo fix this by copying the meshes from the old dataset to the new one
    if (!m_pActiveRenderWin->GetRendererMeshes().empty()) {
      QMessageBox::warning(this, "File Error", "Cropping datasets that contain meshes is not supported at the moment.", QMessageBox::Ok);
      return;
    }

    bool bKeepOldData = (QMessageBox::Yes == 
      QMessageBox::question(NULL, "Create Backup?", "Do you want to create a backup of the current dataset before cropping?", QMessageBox::Yes, QMessageBox::No));


    m_pActiveRenderWin->SetDatasetIsInvalid(true);

    PleaseWaitDialog pleaseWait(this);
    pleaseWait.SetText("Cropping dataset");
    pleaseWait.AttachLabel(&m_MasterController);

    LuaClassInstance first3DRegion = m_pActiveRenderWin->GetFirst3DRegion();
    ExtendedPlane p = m_pActiveRenderWin->GetRendererClipPlane();
    FLOATMATRIX4 trans = m_pActiveRenderWin->GetRotation(first3DRegion) *
                         m_pActiveRenderWin->GetTranslation(first3DRegion);

    // get rid of the viewing transformation in the plane
    p.Transform(trans.inverse(),false);

    if (!m_pActiveRenderWin->RendererCropDataset(m_strTempDir,bKeepOldData)) {
      if (!m_bScriptMode) {
        QMessageBox::warning(this, "File Error", "Unable to crop dataset, is the file write protected?", QMessageBox::Ok);
      }
    } else {
      ToggleClipPlane(false);

    }
    m_pActiveRenderWin->SetDatasetIsInvalid(false);

    RenderWindow* current = m_pActiveRenderWin;
    m_pActiveRenderWin = NULL; // set m_pActiveRenderWin to NULL so RenderWindowActive thinks it has changed
    RenderWindowActive(current);

    pleaseWait.close();
  }
}
