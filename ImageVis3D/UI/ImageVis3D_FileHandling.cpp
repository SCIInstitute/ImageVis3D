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

#include "ImageVis3D.h"
#include "BrowseData.h"

#include <QtCore/QTimer>
#include <QtGui/QMdiSubWindow>
#include <QtGui/QFileDialog>
#include <QtCore/QSettings>
#include <QtGui/QInputDialog>
#include <QtGui/QColorDialog>

#include "PleaseWait.h"

#include <fstream>
#include <iostream>
#include <string>
#include "../Tuvok/Basics/SysTools.h"
#include "../Tuvok/IO/IOManager.h"

#include "DebugOut/QTLabelOut.h"
#include "LODDlg.h"

using namespace std;

// ******************************************
// Dataset
// ******************************************

void MainWindow::LoadDataset() {
  QFileDialog::Options options;
#ifdef TUVOK_OS_APPLE
  options |= QFileDialog::DontUseNativeDialog;
#endif
  QString selectedFilter;

  QSettings settings;
  QString strLastDir = settings.value("Folders/LoadDataset", ".").toString();

  QString dialogString = m_MasterController.IOMan()->GetLoadDialogString().c_str();

  QString fileName = QFileDialog::getOpenFileName(this,
               "Load Dataset", strLastDir,
               dialogString,&selectedFilter, options);

  if (!fileName.isEmpty()) {
    settings.setValue("Folders/LoadDataset", QFileInfo(fileName).absoluteDir().path());
    LoadDataset(fileName);
  };
}


QString MainWindow::GetConvFilename() {
    QFileDialog::Options options;
    #ifdef TUVOK_OS_APPLE
      options |= QFileDialog::DontUseNativeDialog;
    #endif
    QString selectedFilter;

    QSettings settings;
    QString strLastDir = settings.value("Folders/GetConvFilename", ".").toString();

    QString targetFilename =
      QFileDialog::getSaveFileName(this, "Select filename for converted data",
           strLastDir,
           "Universal Volume Format (*.uvf)",&selectedFilter, options);

    if (!targetFilename.isEmpty()) {
      targetFilename = SysTools::CheckExt(string(targetFilename.toAscii()), "uvf").c_str();
      settings.setValue("Folders/GetConvFilename", QFileInfo(targetFilename).absoluteDir().path());
    }

    return targetFilename;
}


bool MainWindow::LoadDataset(const std::vector< std::string >& strParams) {
  if (strParams.size() < 1 || strParams.size() > 2)  return false;
  string inFile = strParams[0], convFile;
  if (strParams.size() == 1)  {
    convFile = SysTools::ChangeExt(inFile, "uvf");
  } else convFile = strParams[1];

  return LoadDataset(inFile.c_str(), convFile.c_str(), true);
}

bool MainWindow::LoadDataset(QString filename, QString targetFilename, bool bNoUserInteraction) {
  PleaseWaitDialog pleaseWait(this);

  if (!filename.isEmpty()) {

    if (!SysTools::FileExists(string(filename.toAscii()))) {
        QString strText = tr("File %1 not found.").arg(filename);
        m_MasterController.DebugOut()->Error("MainWindow::LoadDataset", strText.toStdString().c_str());
        if (!bNoUserInteraction) ShowCriticalDialog( "Load Error", strText);
        return false;
    }

    bool bChecksumFail=false;
    if ((m_bQuickopen && !m_MasterController.IOMan()->NeedsConversion(filename.toStdString())) ||
        !m_MasterController.IOMan()->NeedsConversion(filename.toStdString(), bChecksumFail)) {

      if (bChecksumFail) {
        QString strText = tr("File %1 appears to be a broken UVF file since the header looks ok but the checksum does not match.").arg(filename);
        m_MasterController.DebugOut()->Error("MainWindow::LoadDataset", strText.toStdString().c_str());
        if (!bNoUserInteraction) ShowCriticalDialog( "Load Error", strText);
        return false;
      }

    } else {
      if (!bNoUserInteraction && targetFilename.isEmpty())
        targetFilename = GetConvFilename();

      if (targetFilename.isEmpty()) return false;
      pleaseWait.SetText("Converting, please wait  ...");
      QTLabelOut* labelOut = pleaseWait.AttachLabel(&m_MasterController);
      labelOut->SetOutput(true, true, true, false);

      if (!m_MasterController.IOMan()->ConvertDataset(filename.toStdString(), targetFilename.toStdString(), bNoUserInteraction)) {
        QString strText = tr("Unable to convert file %1 into %2.").arg(filename).arg(targetFilename);
        m_MasterController.DebugOut()->Error("MainWindow::LoadDataset", strText.toStdString().c_str());
        if (!bNoUserInteraction) ShowCriticalDialog( "Conversion Error", strText);

        pleaseWait.close();
        return false;
      }
      filename = targetFilename;
      pleaseWait.close();
    }


    RenderWindow *renderWin = CreateNewRenderWindow(filename);
    renderWin->GetQtWidget()->show();  // calls RenderWindowActive automatically
    UpdateMenus();

    AddFileToMRUList(filename);
    return true;
  } else return false;
}


void MainWindow::LoadDirectory() {
  PleaseWaitDialog pleaseWait(this);

  QSettings settings;
  QString strLastDir = settings.value("Folders/LoadDirectory", ".").toString();

  QString directoryName =
    QFileDialog::getExistingDirectory(this, "Load Dataset from Directory",strLastDir);

  if (!directoryName.isEmpty()) {
    pleaseWait.SetText("Scanning directory for files, please wait  ...");
    settings.setValue("Folders/LoadDirectory", directoryName);
    BrowseData browseDataDialog(m_MasterController, (QDialog*)&pleaseWait,directoryName, this);

    if (browseDataDialog.DataFound()) {
      if (browseDataDialog.exec() == QDialog::Accepted) {

        QFileDialog::Options options;
      #ifdef TUVOK_OS_APPLE
        options |= QFileDialog::DontUseNativeDialog;
      #endif
        QString selectedFilter;

        QString targetFilename = GetConvFilename();
        if (targetFilename.isEmpty()) return;

        pleaseWait.SetText("Converting, please wait  ...");
        if (!m_MasterController.IOMan()->ConvertDataset(browseDataDialog.GetStackInfo(), targetFilename.toStdString())) {
          QString strText =
            tr("Unable to convert file stack from directory %1 into %2.").arg(directoryName).arg(targetFilename);
          ShowCriticalDialog( "Conversion Error", strText);
          m_MasterController.DebugOut()->Error("MainWindow::LoadDirectory", strText.toStdString().c_str());
        }

        RenderWindow *renderWin = CreateNewRenderWindow(targetFilename);
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

bool MainWindow::ExportDataset(UINT32 iLODLevel, std::string targetFileName) {
  if (!m_pActiveRenderWin) {
    m_MasterController.DebugOut()->Warning("MainWindow::ExportDataset", "No active renderwin");
    return false;
  }
  PleaseWaitDialog pleaseWait(this);
  pleaseWait.SetText("Exporting, please wait  ...");
  pleaseWait.AttachLabel(&m_MasterController);

  bool bResult = m_MasterController.IOMan()->ExportDataset(m_pActiveRenderWin->GetRenderer()->GetDataSet(),
                                                 iLODLevel,targetFileName, 
                                                 SysTools::GetPath(targetFileName));  /// \todo maybe come up with something smarter for a temp dir then the target dir
  pleaseWait.close();
  return bResult;
}

void MainWindow::ExportDataset() {
  if (!m_pActiveRenderWin) return;
  QFileDialog::Options options;
#ifdef TUVOK_OS_APPLE
  options |= QFileDialog::DontUseNativeDialog;
#endif
  QString selectedFilter;

  QSettings settings;
  QString strLastDir = settings.value("Folders/ExportDataset", ".").toString();

  QString dialogString = m_MasterController.IOMan()->GetExportDialogString().c_str();

  QString fileName =
    QFileDialog::getSaveFileName(this, "Export Current Dataset",
         strLastDir,
         dialogString,&selectedFilter, options);

  if (!fileName.isEmpty()) {
    settings.setValue("Folders/ExportDataset", QFileInfo(fileName).absoluteDir().path());

    string filter = std::string(selectedFilter.toAscii());
    size_t start = filter.find_last_of("*.")+1;
    size_t end = filter.find_last_of(")");
    string ext = filter.substr(start, end-start);
    SysTools::RemoveTailingWhitespace(ext);
    SysTools::RemoveLeadingWhitespace(ext);

    string strCompletefileName = SysTools::CheckExt(string(fileName.toAscii()), ext);

    int iMaxLODLevel = int(m_pActiveRenderWin->GetRenderer()->GetDataSet()->GetInfo()->GetLODLevelCount())-1;

    int iLODLevel = 0;
    if (iMaxLODLevel > 0) {
      int iMinLODLevel = 0;
      vector<QString> vDesc;
      for (int i = iMinLODLevel;i<=iMaxLODLevel;i++) {
        UINTVECTOR3 vLODSize = UINTVECTOR3(m_pActiveRenderWin->GetRenderer()->GetDataSet()->GetInfo()->GetDomainSize(i));
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
    }

  }

}


bool MainWindow::ExportMesh(UINT32 iLODLevel, string targetFileName) {
    if (!m_pActiveRenderWin) {
      m_MasterController.DebugOut()->Warning("MainWindow::ExportMesh", "No active renderwin");
      return false;
    }
    PleaseWaitDialog pleaseWait(this);
    pleaseWait.SetText("Exporting mesh, please wait  ...");
    pleaseWait.AttachLabel(&m_MasterController);

    int iValue = horizontalSlider_Isovalue->value();
    DOUBLEVECTOR3 vfRescaleFactors;
    vfRescaleFactors.x = doubleSpinBox_RescaleX->value();
    vfRescaleFactors.y = doubleSpinBox_RescaleY->value();
    vfRescaleFactors.z = doubleSpinBox_RescaleZ->value();


    bool bResult = m_MasterController.IOMan()->ExtractIsosurface(m_pActiveRenderWin->GetRenderer()->GetDataSet(),
                                                       iLODLevel, iValue, vfRescaleFactors, targetFileName, 
                                                       SysTools::GetPath(targetFileName));  /// \todo maybe come up with something smarter for a temp dir then the target dir
    pleaseWait.close();

    return bResult;
}

void MainWindow::ExportMesh() {
  QFileDialog::Options options;
#ifdef TUVOK_OS_APPLE
  options |= QFileDialog::DontUseNativeDialog;
#endif
  QString selectedFilter;

  QSettings settings;
  QString strLastDir = settings.value("Folders/ExportMesh", ".").toString();

  QString fileName =
    QFileDialog::getSaveFileName(this, "Export Current Isosurface to Mesh",
         strLastDir,
         "Wavefront OBJ file (*.obj)",&selectedFilter, options);

  if (!fileName.isEmpty()) {
    settings.setValue("Folders/ExportMesh", QFileInfo(fileName).absoluteDir().path());
    string targetFileName = SysTools::CheckExt(string(fileName.toAscii()), "obj");

    int iMaxLODLevel = int(m_pActiveRenderWin->GetRenderer()->GetDataSet()->GetInfo()->GetLODLevelCount())-1;

    int iLODLevel = 0;
    if (iMaxLODLevel > 0) {
      int iMinLODLevel = 0;
      vector<QString> vDesc;
      for (int i = iMinLODLevel;i<=iMaxLODLevel;i++) {
        UINTVECTOR3 vLODSize = UINTVECTOR3(m_pActiveRenderWin->GetRenderer()->GetDataSet()->GetInfo()->GetDomainSize(i));
        QString qstrDesc = tr("%1 x %2 x %3").arg(vLODSize.x).arg(vLODSize.y).arg(vLODSize.z);
        vDesc.push_back(qstrDesc);
      }
      LODDlg lodDlg("For which LOD Level do you want to compute the mesh?", iMinLODLevel, iMaxLODLevel, vDesc, this);

      if (lodDlg.exec() != QDialog::Accepted) 
        return;
      else
        iLODLevel = lodDlg.GetLOD();
    }

    if(!ExportMesh(UINT32(iLODLevel), targetFileName))
      ShowCriticalDialog( "Error during mesh export.", "The system was unable to export the current data set, please check the error log for details (Menu -> \"Help\" -> \"Debug Window\").");
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
