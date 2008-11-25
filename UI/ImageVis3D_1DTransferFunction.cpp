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


//!    File   : ImageVis3D_1DTransferFunction.cpp
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
// 1D Transfer Function Dock
// ******************************************

void MainWindow::Transfer1DSetColors() {

  radioButton_User->setChecked(true);

  unsigned int iPaintMode = Q1DTransferFunction::PAINT_NONE;

  if (checkBox_Red->isChecked() )
    iPaintMode |= Q1DTransferFunction::PAINT_RED;
  if (checkBox_Green->isChecked() )
    iPaintMode |= Q1DTransferFunction::PAINT_GREEN;
  if (checkBox_Blue->isChecked() )
    iPaintMode |= Q1DTransferFunction::PAINT_BLUE;
  if (checkBox_Alpha->isChecked() )
    iPaintMode |= Q1DTransferFunction::PAINT_ALPHA;

  m_1DTransferFunction->
    SetPaintMode( (Q1DTransferFunction::paintMode ) iPaintMode);
}

void MainWindow::Transfer1DSetGroups() {

  // Determine current CB state
  unsigned int iRadioState = 0;

  if (radioButton_User->isChecked())
    iRadioState = 0;
  else if (radioButton_Luminance->isChecked())
    iRadioState = 1;
  else //if (radioButton_Intensity->isChecked())
    iRadioState = 2;

  // If in user mode do nothing
  if (iRadioState == 0) return;

  // apply iRadioState
  checkBox_Red->setChecked(true);  
  checkBox_Green->setChecked(true);  
  checkBox_Blue->setChecked(true);  
  checkBox_Alpha->setChecked(iRadioState==2);

  unsigned int iPaintMode = (Q1DTransferFunction::PAINT_RED |
			     Q1DTransferFunction::PAINT_GREEN |
			     Q1DTransferFunction::PAINT_BLUE |
			     ((iRadioState==2) ?
			      Q1DTransferFunction::PAINT_ALPHA :
			      Q1DTransferFunction::PAINT_NONE) );

  m_1DTransferFunction->
    SetPaintMode( (Q1DTransferFunction::paintMode ) iPaintMode);
}

void MainWindow::SetUpdateMode() {
  if( radioButton_UpdateContinuous->isChecked() ) {
    m_1DTransferFunction->SetExecutionMode( QTransferFunction::CONTINUOUS );
    m_2DTransferFunction->SetExecutionMode( QTransferFunction::CONTINUOUS );
  } else if( radioButton_UpdateOnRelease->isChecked() ) {
    m_1DTransferFunction->SetExecutionMode( QTransferFunction::ONRELEASE );
    m_2DTransferFunction->SetExecutionMode( QTransferFunction::ONRELEASE );
  } else if( radioButton_UpdateManual->isChecked() ) {
    m_1DTransferFunction->SetExecutionMode( QTransferFunction::MANUAL );
    m_2DTransferFunction->SetExecutionMode( QTransferFunction::MANUAL );
  }
  else {
    m_1DTransferFunction->SetExecutionMode( QTransferFunction::UNKNOWN );
    m_2DTransferFunction->SetExecutionMode( QTransferFunction::UNKNOWN );
  }

  pushButton_ApplyUpdate->setEnabled(radioButton_UpdateManual->isChecked());
}

void MainWindow::ApplyUpdate() {
  if (radioButton_1DTrans->isChecked()) m_1DTransferFunction->ApplyFunction();
  if (radioButton_2DTrans->isChecked()) m_2DTransferFunction->ApplyFunction();
}

void MainWindow::Transfer1DLoad() {
  QSettings settings;
  QString strLastDir = settings.value("Folders/Transfer1DLoad", ".").toString();

  QString fileName =
    QFileDialog::getOpenFileName(this,
				 "Load 1D Transferfunction", strLastDir,
				 "1D Transferfunction File (*.1dt)");

  if (!fileName.isEmpty()) {
    settings.setValue("Folders/Transfer1DLoad", QFileInfo(fileName).absoluteDir().path());
    m_1DTransferFunction->LoadFromFile(fileName);
  }
}

void MainWindow::Transfer1DSave() {
  QSettings settings;
  QString strLastDir = settings.value("Folders/Transfer1DSave", ".").toString();

  QString fileName =
    QFileDialog::getSaveFileName(this,
				 "Save 1D Transferfunction", strLastDir,
				 "1D Transferfunction File (*.1dt)");

  if (!fileName.isEmpty()) {
    settings.setValue("Folders/Transfer1DSave", QFileInfo(fileName).absoluteDir().path());
    m_1DTransferFunction->SaveToFile(fileName);
  }
}

void MainWindow::Transfer1DCopyTo2DTrans() {
  m_2DTransferFunction->Set1DTrans(m_1DTransferFunction->GetTrans());
}
