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

#include "DebugOut/QTLabelOut.h"
#include "../Tuvok/DebugOut/MultiplexOut.h"

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

    if (!targetFilename.isEmpty())
      settings.setValue("Folders/GetConvFilename", QFileInfo(targetFilename).absoluteDir().path());

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

      // add status label into debug chain
      AbstrDebugOut* pOldDebug       = m_MasterController.DebugOut();
      bool           bDeleteOldDebug = m_MasterController.DoDeleteDebugOut();
      m_MasterController.SetDeleteDebugOut(false);

      MultiplexOut* pMultiOut = new MultiplexOut();
      m_MasterController.SetDebugOut(pMultiOut, true);

      QTLabelOut* labelOut = new QTLabelOut(pleaseWait.GetStatusLabel(),
                                            &pleaseWait);
      labelOut->SetOutput(true, true, true, false);

      pMultiOut->AddDebugOut(labelOut,  true);
      pMultiOut->AddDebugOut(pOldDebug, false);
     
      if (!m_MasterController.IOMan()->ConvertDataset(filename.toStdString(), targetFilename.toStdString(), bNoUserInteraction)) {
        QString strText = tr("Unable to convert file %1 into %2.").arg(filename).arg(targetFilename);
        m_MasterController.DebugOut()->Error("MainWindow::LoadDataset", strText.toStdString().c_str());
        if (!bNoUserInteraction) ShowCriticalDialog( "Conversion Error", strText);

        m_MasterController.SetDebugOut(pOldDebug, bDeleteOldDebug);
        return false;
      }      
      filename = targetFilename;
      pleaseWait.close();
      m_MasterController.SetDebugOut(pOldDebug,bDeleteOldDebug);
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


void MainWindow::SaveDataset() {
  QFileDialog::Options options;
#ifdef TUVOK_OS_APPLE
  options |= QFileDialog::DontUseNativeDialog;
#endif
  QString selectedFilter;

  QSettings settings;
  QString strLastDir = settings.value("Folders/SaveDataset", ".").toString();

  QString fileName =
    QFileDialog::getSaveFileName(this, "Save Current Dataset",
         strLastDir,
         "Universal Volume Format (*.uvf)",&selectedFilter, options);

  if (!fileName.isEmpty())
    settings.setValue("Folders/SaveDataset", QFileInfo(fileName).absoluteDir().path());

}
