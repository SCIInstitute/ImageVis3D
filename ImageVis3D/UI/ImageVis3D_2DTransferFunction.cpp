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


//!    File   : ImageVis3D_2DTransferFunction.cpp
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

using namespace std;



// ******************************************
// 2D Transfer Function Dock
// ******************************************

void MainWindow::Transfer2DSwatchesChanged() {

  listWidget_Swatches->clear();

  for (size_t i = 0;i<m_2DTransferFunction->GetSwatchCount();i++) {

    size_t iSize = m_2DTransferFunction->GetSwatchSize(uint32_t(i));

    QString msg;
    switch (iSize) {
    case  3  : msg = "Triangle";  break;
    case  4  : msg = "Quadrilateral";  break;
    case  5  : msg = "Pentagon";  break;
    case  6  : msg = "Hexagon";    break;
    case  7  : msg = "Heptagon";  break;
    case  8  : msg = "Octagon";    break;
    case  9  : msg = "Nonagon";    break;
    case 10  : msg = "Decagon";    break;
    case 11  : msg = "Hendecagon";  break;
    case 12  : msg = "Dodecagon";  break;
    case 13  : msg = "Triskaidecagon";  break;
    case 14  : msg = "Tetrakaidecagon";  break;
    case 15  : msg = "Pendedecagon";  break;
    case 16  : msg = "Hexdecagon";  break;
    case 17  : msg = "Heptdecagon";  break;
    case 18  : msg = "Octdecagon";  break;
    case 19  : msg = "Enneadecagon";  break;
    case 20  : msg = "Icosagon";  break;
      // at this point I am getting bored ...
    default : msg = tr("%1 - gon").arg(iSize); break;
    }

    listWidget_Swatches->addItem( msg );
  }

  int iCurrent = m_2DTransferFunction->GetActiveSwatchIndex();
  listWidget_Swatches->setCurrentRow(iCurrent);

  Transfer2DUpdateSwatchButtons();
  UpdatePolyTypeLabel(iCurrent);
}

void MainWindow::Transfer2DSwatcheTypeChanged(int iIndex) {
  int iCurrent = m_2DTransferFunction->GetActiveSwatchIndex();
  if (iIndex == iCurrent) UpdatePolyTypeLabel(iCurrent);
}


void MainWindow::Transfer2DUpdateGradientType() {
  bool b = m_2DTransferFunction->GetActiveGradientType();
  if (b)
    radioButton_radGrad->setChecked(true);
  else
    radioButton_linGrad->setChecked(true);
}

void MainWindow::Transfer2DToggleGradientType() {
  m_2DTransferFunction->SetActiveGradientType(radioButton_radGrad->isChecked());
}

void MainWindow::Transfer2DUpdateSwatchButtons() {

  int iCurrent = m_2DTransferFunction->GetActiveSwatchIndex();

  pushButton_DelPoly->setEnabled(iCurrent >= 0);
  pushButton_UpPoly->setEnabled(iCurrent > 0);
  pushButton_DownPoly->
    setEnabled(iCurrent < int(m_2DTransferFunction->GetSwatchCount())-1);

  Transfer2DUpdateGradientBox();
}


void MainWindow::Transfer2DUpdateGradientBox() {

  int iCurrent = listWidget_Gradient->currentRow();

  listWidget_Gradient->clear();

  if (m_2DTransferFunction->GetActiveSwatchIndex() > -1) {

    for (size_t i = 0;i<m_2DTransferFunction->GetGradientCount();i++) {
      GradientStop s =  m_2DTransferFunction->GetGradient(uint32_t(i));
      QString msg = tr("Stop at %1").arg(s.first);
      listWidget_Gradient->addItem( msg );
    }

    listWidget_Gradient->
      setCurrentRow(min<int>(iCurrent,
           int(m_2DTransferFunction->GetGradientCount())));

  }

  Transfer2DUpdateGradientButtons();
  Transfer2DUpdateGradientType();
}


void MainWindow::Transfer2DUpdateGradientButtons() {
  pushButton_AddStop->setEnabled(m_2DTransferFunction->GetActiveSwatchIndex() != -1);

  int iCurrent;
  if ( m_2DTransferFunction->Get2DTFMode() == TFM_EXPERT)
    iCurrent = listWidget_Gradient->currentRow();
  else
    iCurrent = min<int>(1,int(m_2DTransferFunction->GetGradientCount())-1);

  pushButton_DelStop->setEnabled(iCurrent >= 0);
  frame_ChooseColor->setEnabled(iCurrent >= 0);

  if (iCurrent >= 0 && m_2DTransferFunction->GetSwatchCount() > 0) {

    GradientStop s =
      m_2DTransferFunction->GetGradient(iCurrent);

    QString strStyle =
      tr("QPushButton { background: rgb(%1, %2, %3); color: rgb(%4, %5, %6) }").arg(int(s.second[0]*255)).arg(int(s.second[1]*255)).arg(int(s.second[2]*255)).arg(int((1-s.second[0])*255)).arg(int((1-s.second[1])*255)).arg(int((1-s.second[2])*255));

    if ( m_2DTransferFunction->Get2DTFMode() == TFM_EXPERT) {
      pushButton_ColorChooser->setStyleSheet( strStyle );
      horizontalSlider_Opacity->setValue(int(s.second[3]*100));
    } else {
      pushButton_ColorChooser_SimpleUI->setStyleSheet( strStyle );
      horizontalSlider_Opacity_SimpleUI->setValue(int(s.second[3]*100));
    }

  } else {
    if ( m_2DTransferFunction->Get2DTFMode() == TFM_EXPERT) {
      pushButton_ColorChooser->setStyleSheet( "" );
      horizontalSlider_Opacity->setValue(0);
    } else {
      pushButton_ColorChooser_SimpleUI->setStyleSheet( "" );
      horizontalSlider_Opacity_SimpleUI->setValue(0);
    }
  }
}


void MainWindow::Transfer2DAddGradient() {

  bool ok;
  float f = float(QInputDialog::getDouble(this,
            tr("Select Gradient Stop"),
            tr("Stop at:"), 0.5, 0, 1, 3, &ok));

  if (!ok) return;

  GradientStop s(f, FLOATVECTOR4(1,1,1,1));
  m_2DTransferFunction->AddGradient(s);

  Transfer2DUpdateGradientBox();
}


void MainWindow::Transfer2DChooseGradientColor() {

  GradientStop s =
    m_2DTransferFunction->GetGradient(listWidget_Gradient->currentRow());


  const int old_color[3] = {
    static_cast<int>(s.second[0] * 255.f),
    static_cast<int>(s.second[1] * 255.f),
    static_cast<int>(s.second[2] * 255.f)
  };
  QColor prevColor(old_color[0], old_color[1], old_color[2]);
  QColor color = QColorDialog::getColor(prevColor, this);

  if (color.isValid()) {
    s.second[0] = color.red()/255.0f;
    s.second[1] = color.green()/255.0f;
    s.second[2] = color.blue()/255.0f;

    m_2DTransferFunction->SetGradient(listWidget_Gradient->currentRow(),s);

    Transfer2DUpdateGradientButtons();
  }
}

void MainWindow::Transfer2DChooseGradientOpacity() {
  GradientStop s =
    m_2DTransferFunction->GetGradient(listWidget_Gradient->currentRow());
  s.second[3] = horizontalSlider_Opacity->value()/100.0f;
  m_2DTransferFunction->SetGradient(listWidget_Gradient->currentRow(),s);
}


void MainWindow::Transfer2DChooseGradientColorSimpleUI() {
  if (m_2DTransferFunction->GetActiveSwatchIndex() == -1) return;
  int iIndex = min<int>(1,int(m_2DTransferFunction->GetGradientCount())-1);
  if (iIndex < 0) return;

  GradientStop s = m_2DTransferFunction->GetGradient(iIndex);

  const int old_color[3] = {
    static_cast<int>(s.second[0] * 255.f),
    static_cast<int>(s.second[1] * 255.f),
    static_cast<int>(s.second[2] * 255.f)
  };
  QColor prevColor(old_color[0], old_color[1], old_color[2]);
  QColor color = QColorDialog::getColor(prevColor, this);

  if (color.isValid()) {
    s.second[0] = color.red()/255.0f;
    s.second[1] = color.green()/255.0f;
    s.second[2] = color.blue()/255.0f;

    m_2DTransferFunction->SetGradient(iIndex,s);

    Transfer2DUpdateGradientButtons();
  }
}

void MainWindow::Transfer2DChooseGradientOpacitySimpleUI() {
  if (m_2DTransferFunction->GetActiveSwatchIndex() == -1) return;
  int iIndex = min<int>(1,int(m_2DTransferFunction->GetGradientCount())-1);
  if (iIndex < 0) return;

  GradientStop s = m_2DTransferFunction->GetGradient(iIndex);
  s.second[3] = horizontalSlider_Opacity_SimpleUI->value()/100.0f;
  m_2DTransferFunction->SetGradient(iIndex,s);
}


void MainWindow::Transfer2DDeleteGradient() {

  m_2DTransferFunction->DeleteGradient(listWidget_Gradient->currentRow());
  Transfer2DUpdateGradientBox();
}


bool MainWindow::Transfer2DLoad(const std::wstring& strFilename) {
  return (m_2DTransferFunction) ? m_2DTransferFunction->LoadFromFile(QString::fromStdWString(strFilename)) : false;
}

void MainWindow::Transfer2DLoad() {
  QSettings settings;
  QString strLastDir;

  shared_ptr<LuaScripting> ss = m_MasterController.LuaScript();

  // First try to grab the directory from the currently-opened file.
  if(m_pActiveRenderWin) {
    LuaClassInstance ds = m_pActiveRenderWin->GetRendererDataset();
    strLastDir = QString::fromStdWString(SysTools::GetPath(
            ss->cexecRet<wstring>(ds.fqName() + ".fullpath")));
  }

  // if that didn't work, fall back on our previously saved path.
  if(strLastDir == "" || !SysTools::FileExists(strLastDir.toStdWString())) {
    strLastDir = settings.value("Folders/Transfer2DLoad", ".").toString();
  }

  QString selectedFilter;
  QFileDialog::Options options;
#ifdef DETECTED_OS_APPLE
  options |= QFileDialog::DontUseNativeDialog;
#endif

  QString fileName =
    QFileDialog::getOpenFileName(this, "Load 2D Transfer function", strLastDir,
         "2D Transfer function File (*.2dt)",&selectedFilter, options);

  if (!fileName.isEmpty()) {
    settings.setValue("Folders/Transfer2DLoad", QFileInfo(fileName).absoluteDir().path());
    m_2DTransferFunction->LoadFromFile(fileName);
  }
}

void MainWindow::LoadTransferFunction2D(const std::wstring& tf) {
  m_2DTransferFunction->LoadFromFile(QString::fromStdWString(tf));
}

void MainWindow::Transfer2DSave() {
  QSettings settings;
  QString strLastDir="";
  std::wstring defaultFilename;

  shared_ptr<LuaScripting> ss = m_MasterController.LuaScript();

  // First try to grab the directory from the currently-opened file.
  if(m_pActiveRenderWin) {
    LuaClassInstance ds = m_pActiveRenderWin->GetRendererDataset();
    wstring dsFullpath = ss->cexecRet<wstring>(ds.fqName() + ".fullpath");
    strLastDir = QString::fromStdWString(SysTools::GetPath(dsFullpath));
    defaultFilename = SysTools::ChangeExt(dsFullpath, L"2dt");
  }

  // if that didn't work, fall back on our previously saved path.
  if(strLastDir == "" || !SysTools::FileExists(strLastDir.toStdWString()+L".")) {
    // ...if that didn't work, fall back on our previously saved path.
    strLastDir = settings.value("Folders/Transfer2DSave", ".").toString();
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
    QFileDialog::getSaveFileName(this, "Save 2D Transfer function", strLastDir,
         "2D Transfer function File (*.2dt)",&selectedFilter, options);

  if (!fileName.isEmpty()) {
    fileName = QString::fromStdWString(SysTools::CheckExt(fileName.toStdWString(), L"2dt"));
    settings.setValue("Folders/Transfer2DSave", QFileInfo(fileName).absoluteDir().path());
    m_2DTransferFunction->SaveToFile(fileName);
  }
}


void MainWindow::Transfer2DToggleTFMode() {
  m_2DTransferFunction->Toggle2DTFMode();
  E2DTransferFunctionMode tfMode = m_2DTransferFunction->Get2DTFMode();

  if ( tfMode  == TFM_EXPERT ) {
    listWidget_Gradient->setCurrentRow(min<int>(1,int(m_2DTransferFunction->GetGradientCount())-1));

    frame_Simple2DTransControls->setVisible(false);
    frame_Expert2DTransControls->setVisible(true);
  } else {
    frame_Simple2DTransControls->setVisible(true);
    frame_Expert2DTransControls->setVisible(false);

    UpdatePolyTypeLabel(m_2DTransferFunction->GetActiveSwatchIndex());
  }

  Transfer2DUpdateGradientButtons();

  QSettings settings;
  settings.setValue("UI/2DTFMode", int(tfMode));
}

void MainWindow::UpdatePolyTypeLabel(int iCurrent) {
  if (iCurrent >= 0)
    pushButton_DelPoly_SimpleUI->setText(tr("Delete current %1").arg(m_2DTransferFunction->GetSwatchDesciption().c_str()));
  else
    pushButton_DelPoly_SimpleUI->setText("Delete");
}
