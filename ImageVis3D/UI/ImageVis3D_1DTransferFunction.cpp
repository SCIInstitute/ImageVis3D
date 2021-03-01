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
#include <QtWidgets/QMdiSubWindow>
#include <QtWidgets/QFileDialog>
#include <QtCore/QSettings>
#include <QtWidgets/QInputDialog>
#include <QtWidgets/QColorDialog>

#include "PleaseWait.h"

#include <fstream>
#include <iostream>
#include <string>
#include "../Tuvok/Basics/SysTools.h"
#include "../Tuvok/IO/FileBackedDataset.h"
#include "../Tuvok/LuaScripting/TuvokSpecific/LuaTransferFun1DProxy.h"

using namespace std;

// ******************************************
// 1D Transfer Function Dock
// ******************************************

void MainWindow::Invert1DTransComp() {
  checkBox_Red->setChecked(!checkBox_Red->isChecked());
  checkBox_Green->setChecked(!checkBox_Green->isChecked());
  checkBox_Blue->setChecked(!checkBox_Blue->isChecked());
  checkBox_Alpha->setChecked(!checkBox_Alpha->isChecked());

  Transfer1DSetColors();
}

void MainWindow::Transfer1DSetColors() {

  radioButton_User->setChecked(true);

  unsigned int iPaintMode = Q1DTransferFunction::PAINT_NONE;

  if (!checkBox_Red->isChecked() && 
      !checkBox_Green->isChecked() &&
      !checkBox_Blue->isChecked() &&
      !checkBox_Alpha->isChecked() ) {
      // everything disable -> not a good idea

    groupBox_6->setStyleSheet("background-color: red");

  } else {
    groupBox_6->setStyleSheet("");

    if (checkBox_Red->isChecked() )
      iPaintMode |= Q1DTransferFunction::PAINT_RED;
    if (checkBox_Green->isChecked() )
      iPaintMode |= Q1DTransferFunction::PAINT_GREEN;
    if (checkBox_Blue->isChecked() )
      iPaintMode |= Q1DTransferFunction::PAINT_BLUE;
    if (checkBox_Alpha->isChecked() )
      iPaintMode |= Q1DTransferFunction::PAINT_ALPHA;
  }


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

  Transfer1DSetColors();
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

bool MainWindow::Transfer1DLoad(const std::wstring& strFilename) {
  return (m_1DTransferFunction) ? m_1DTransferFunction->LoadFromFile(strFilename) : false;
}

void MainWindow::Transfer1DLoad() {
  QSettings settings;
  QString strLastDir="";

  shared_ptr<LuaScripting> ss = m_MasterController.LuaScript();

  // First try to grab the directory from the currently-opened file.
  if(m_pActiveRenderWin) {
    LuaClassInstance ds = m_pActiveRenderWin->GetRendererDataset();
    strLastDir = QString::fromStdWString(SysTools::GetPath(
            ss->cexecRet<wstring>(ds.fqName() + ".fullpath")));
  }

  // if that didn't work, fall back on our previously saved path.
  if(strLastDir == "" || !SysTools::FileExists(strLastDir.toStdWString())) {
    strLastDir = settings.value("Folders/Transfer1DLoad", ".").toString();
  }

  QString selectedFilter;
  QFileDialog::Options options;
#ifdef DETECTED_OS_APPLE
  options |= QFileDialog::DontUseNativeDialog;
#endif

  QString fileName =
    QFileDialog::getOpenFileName(this,
         "Load 1D Transfer function", strLastDir,
         "1D Transfer function File (*.1dt)",&selectedFilter, options);

  if (!fileName.isEmpty()) {
    settings.setValue("Folders/Transfer1DLoad", QFileInfo(fileName).absoluteDir().path());
    m_1DTransferFunction->QLoadFromFile(fileName);
  }
}

void MainWindow::LoadTransferFunction1D(const std::wstring& tf) {
  m_1DTransferFunction->LoadFromFile(tf);
}

void MainWindow::Transfer1DSave() {
  QSettings settings;
  QString strLastDir="";
  std::wstring defaultFilename;
  
  shared_ptr<LuaScripting> ss = m_MasterController.LuaScript();

  // First try to grab the directory from the currently-opened file...
  if(m_pActiveRenderWin) {
    LuaClassInstance ds = m_pActiveRenderWin->GetRendererDataset();
    wstring dsFullpath = ss->cexecRet<wstring>(ds.fqName() + ".fullpath");
    strLastDir = QString::fromStdWString(SysTools::GetPath(dsFullpath));
    defaultFilename = SysTools::ChangeExt(dsFullpath, L"1dt");
  }

  if(strLastDir == "" || !SysTools::FileExists(strLastDir.toStdWString()+L".")) {
    // ...if that didn't work, fall back on our previously saved path.
    strLastDir = settings.value("Folders/Transfer1DSave", ".").toString();
  } else {
    // if the path exitst propose the default name as save default
    strLastDir = QString::fromStdWString(defaultFilename);
  }

  QString selectedFilter;
  QFileDialog::Options options;
#ifdef DETECTED_OS_APPLE
  options |= QFileDialog::DontUseNativeDialog;
#endif

  QString fileName =
    QFileDialog::getSaveFileName(this,
         "Save 1D Transfer function", strLastDir,
         "1D Transfer function File (*.1dt)",&selectedFilter, options);

  if (!fileName.isEmpty()) {
    fileName = QString::fromStdWString(SysTools::CheckExt(fileName.toStdWString(), L"1dt"));
    settings.setValue("Folders/Transfer1DSave", QFileInfo(fileName).absoluteDir().path());
    m_1DTransferFunction->SaveToFile(fileName);
  }
}

void MainWindow::Transfer1DCopyTo2DTrans() {
  m_2DTransferFunction->Set1DTrans(m_1DTransferFunction->GetTrans());
}


void MainWindow::Populate1DTFLibList() {
  comboBox_1DTFLib->clear();

  string strHomeDir = ".";
  if (!SysTools::GetHomeDirectory(strHomeDir)) {
    m_MasterController.DebugOut()->Warning("MainWindow::Populate1DTFLibList", "Unable to determine user's home directory!");
  }

  QSettings settings;
  QString strLibDir = settings.value("Folders/Transfer1DLib", strHomeDir.c_str()).toString();
  settings.setValue("Folders/Transfer1DLib", strLibDir);


  std::vector<std::wstring> files = SysTools::GetDirContents(strLibDir.toStdWString(), L"*", L"1dt");
  for (size_t i = 0; i < files.size(); i++) {
    std::wstring desc = SysTools::RemoveExt(SysTools::GetFilename(files[i]));
    comboBox_1DTFLib->insertItem(0, QString::fromStdWString(desc), QString::fromStdWString(files[i]));
  }
}

void MainWindow::Transfer1DAddToLib() {
  QSettings settings;
  QString strLibDir = settings.value("Folders/Transfer1DLib", ".").toString();

  QString selectedFilter;
  QFileDialog::Options options;
#ifdef DETECTED_OS_APPLE
  options |= QFileDialog::DontUseNativeDialog;
#endif

  QString fileName =
    QFileDialog::getSaveFileName(this,
         "Save 1D Transfer function", strLibDir,
         "1D Transfer function File (*.1dt)",&selectedFilter, options);

  if (!fileName.isEmpty()) {
    fileName = QString::fromStdWString(SysTools::CheckExt(fileName.toStdWString(), L"1dt"));
    settings.setValue("Folders/Transfer1DSave", QFileInfo(fileName).absoluteDir().path());
    m_1DTransferFunction->SaveToFile(fileName);
    Populate1DTFLibList();
  }
}

void MainWindow::Transfer1DSetFromLib() {
  if (comboBox_1DTFLib->currentIndex() >= 0 && comboBox_1DTFLib->currentIndex() < comboBox_1DTFLib->count()) {
    QFile file(comboBox_1DTFLib->itemData(comboBox_1DTFLib->currentIndex()).toString());
    if (file.exists())
      m_1DTransferFunction->QLoadFromFile(file.fileName());
  }
}

void MainWindow::Transfer1DAddFromLib() {
  if (comboBox_1DTFLib->currentIndex() >= 0 && comboBox_1DTFLib->currentIndex() < comboBox_1DTFLib->count()) {
    QFile file(comboBox_1DTFLib->itemData(comboBox_1DTFLib->currentIndex()).toString());
    if (file.exists())
      m_1DTransferFunction->AddFromFile(file.fileName());
  }
}

void MainWindow::Transfer1DSubFromLib() {
  if (comboBox_1DTFLib->currentIndex() >= 0 && comboBox_1DTFLib->currentIndex() < comboBox_1DTFLib->count()) {
    QFile file(comboBox_1DTFLib->itemData(comboBox_1DTFLib->currentIndex()).toString());
    if (file.exists())
      m_1DTransferFunction->SubtractFromFile(file.fileName());
  }
}

void MainWindow::Transfer1DConfigureLib() {
  QSettings settings;
  QString strLibDir = settings.value("Folders/Transfer1DLib", ".").toString();

  QString strNewLibDir =
    QFileDialog::getExistingDirectory(this, "Select transfer function library",strLibDir);

  if (!strNewLibDir.isEmpty()) {
    if (strNewLibDir[strNewLibDir.size()-1] != '/' &&
        strNewLibDir[strNewLibDir.size()-1] != '\\') strNewLibDir = strNewLibDir+'/';
  
    if (strNewLibDir != strLibDir) {
      settings.setValue("Folders/Transfer1DLib", strNewLibDir);
      Populate1DTFLibList();
    }
  }
}
