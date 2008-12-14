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


//!    File   : ImageVis3D_Capturing.cpp
//!    Author : Jens Krueger
//!             SCI Institute
//!             University of Utah
//!    Date   : September 2008
//
//!    Copyright (C) 2008 SCI Institute

#include "ImageVis3D.h"
#include <Basics/SysTools.h>
#include <QtGui/QMessageBox>
#include <QtGui/QFileDialog>
#include <QtCore/QSettings>


using namespace std;

void MainWindow::CaptureFrame() {
  if (m_ActiveRenderWin) {
	  if (!m_ActiveRenderWin->CaptureFrame(lineEditCaptureFile->text().toStdString())) {
		QString msg = tr("Error writing image file %1").arg(lineEditCaptureFile->text());
		QMessageBox::warning(this, tr("Error"), msg);
	  }
  }
}

void MainWindow::CaptureSequence() {
  
  if (m_ActiveRenderWin) {
	  if (!m_ActiveRenderWin->CaptureSequenceFrame(lineEditCaptureFile->text().toStdString())) {
		QString msg = tr("Error writing image file %1").arg(lineEditCaptureFile->text());
		QMessageBox::warning(this, tr("Error"), msg);
	  }
  }
}

void MainWindow::CaptureRotation() {
  if (m_ActiveRenderWin) {
    m_ActiveRenderWin->ToggleHQCaptureMode();
    int iImagesPerAngle = (horizontalSlider_RotSpeed->maximum()+1) - horizontalSlider_RotSpeed->value();
	  int i = 0;
	  float fAngle = 0.0f;
	  while (fAngle < 360) {
      m_ActiveRenderWin->SetCaptureRotationAngle(fAngle);
	    m_ActiveRenderWin->CaptureSequenceFrame(lineEditCaptureFile->text().toStdString());
	    fAngle = float(i) / float(iImagesPerAngle);
	    i++;
	  }
    m_ActiveRenderWin->ToggleHQCaptureMode();
  }
}

void MainWindow::SetCaptureFilename() {
  QFileDialog::Options options;
#if defined(macintosh) || (defined(__MACH__) && defined(__APPLE__))
  options |= QFileDialog::DontUseNativeDialog;
#endif
  QString selectedFilter;

  QSettings settings;
  QString strLastDir = settings.value("Folders/SetCaptureFilename", ".").toString();

  QString fileName = QFileDialog::getSaveFileName(this,"Select Image File", strLastDir,
					   "All Files (*.*)",&selectedFilter, options);


  if (!fileName.isEmpty()) {
	  // add png as the default filetype if the user forgot to enter one
	  if (SysTools::GetExt(fileName.toStdString()) == "")
		  fileName = fileName + ".png";

    settings.setValue("Folders/SetCaptureFilename", QFileInfo(fileName).absoluteDir().path());
	  lineEditCaptureFile->setText(fileName);
  }

}
