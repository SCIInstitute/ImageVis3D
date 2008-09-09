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

using namespace std;

// ******************************************
// Dataset
// ******************************************

void MainWindow::LoadDataset() {
  LoadDataset(QFileDialog::getOpenFileName(this,
					   "Load Dataset", ".",
					   "All known Files (*.dat *.nrrd *.uvf);;QVis Data (*.dat);;Nearly Raw Raster Data (*.nrrd);;Universal Volume Format (*.uvf)"));
}


void MainWindow::LoadDataset(QString fileName) {
  if (!fileName.isEmpty()) {
    RenderWindow *renderWin = CreateNewRenderWindow(fileName);
    renderWin->show();

    AddFileToMRUList(fileName);
  }
}


void MainWindow::LoadDirectory() {
  PleaseWaitDialog pleaseWait(this);

  QString directoryName =
    QFileDialog::getExistingDirectory(this, "Load Dataset from Directory");

  if (!directoryName.isEmpty()) {
    pleaseWait.SetText("Scanning directory for DICOM files, please wait  ...");

    QString fileName;
    BrowseData browseDataDialog((QDialog*)&pleaseWait,directoryName, this);

    if (browseDataDialog.DataFound()) {
      if (browseDataDialog.exec() == QDialog::Accepted) {
        LoadDataset(browseDataDialog.GetFileName());
      }
    } else {
      QString msg =
	      tr("Error no valid DICOM files in directory %1 found.").arg(directoryName);
      QMessageBox::information(this, tr("Problem"), msg);
    }
  }
}


void MainWindow::SaveDataset() {
  QString fileName =
    QFileDialog::getSaveFileName(this, "Save Current Dataset",
				 ".",
				 "Nearly Raw Raster Data (*.nrrd);;QVis Data (*.dat);;Universal Volume Format (*.uvf)");
}
