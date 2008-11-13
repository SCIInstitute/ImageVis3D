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
// 2D Transfer Function Dock
// ******************************************

void MainWindow::Transfer2DSwatchesChanged() {

  listWidget_Swatches->clear();
  
  for (size_t i = 0;i<m_2DTransferFunction->GetSwatchCount();i++) {

    size_t iSize = m_2DTransferFunction->GetSwatchSize(uint(i));

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
  Transfer2DUpdateGradientBox();
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
      GradientStop s =  m_2DTransferFunction->GetGradient(uint(i));
      QString msg = tr("Stop at %1").arg(s.first);
      listWidget_Gradient->addItem( msg );
    }

    listWidget_Gradient->
      setCurrentRow(min<int>(iCurrent,
			     int(m_2DTransferFunction->GetGradientCount())));
    
  }

  Transfer2DUpdateGradientButtons();
}


void MainWindow::Transfer2DUpdateGradientButtons() {

  pushButton_AddStop->
    setEnabled(m_2DTransferFunction->GetActiveSwatchIndex() != -1);

  int iCurrent = listWidget_Gradient->currentRow();

  pushButton_DelStop->setEnabled(iCurrent >= 0);
  frame_ChooseColor->setEnabled(iCurrent >= 0);

  if (iCurrent >= 0 && m_2DTransferFunction->GetSwatchCount() > 0) {

    GradientStop s =
      m_2DTransferFunction->GetGradient(listWidget_Gradient->currentRow());

    QString strStyle =
      tr("QPushButton { background: rgb(%1, %2, %3); color: rgb(%4, %5, %6) }").arg(int(s.second[0]*255)).arg(int(s.second[1]*255)).arg(int(s.second[2]*255)).arg(int((1-s.second[0])*255)).arg(int((1-s.second[1])*255)).arg(int((1-s.second[2])*255));

    pushButton_ColorChooser->setStyleSheet( strStyle );
  
    horizontalSlider_Opacity->setValue(int(s.second[3]*100));

  } else {
    pushButton_ColorChooser->setStyleSheet( "" );
    horizontalSlider_Opacity->setValue(0);
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


  QColor color = QColorDialog::getColor(Qt::green, this);
  s.second[0] = color.red()/255.0f;
  s.second[1] = color.green()/255.0f;
  s.second[2] = color.blue()/255.0f;


  m_2DTransferFunction->SetGradient(listWidget_Gradient->currentRow(),s);

  Transfer2DUpdateGradientButtons();
}


void MainWindow::Transfer2DChooseGradientOpacity() {

  GradientStop s =
    m_2DTransferFunction->GetGradient(listWidget_Gradient->currentRow());
  s.second[3] = horizontalSlider_Opacity->value()/100.0f;
  m_2DTransferFunction->SetGradient(listWidget_Gradient->currentRow(),s);
}


void MainWindow::Transfer2DDeleteGradient() {

  m_2DTransferFunction->DeleteGradient(listWidget_Gradient->currentRow());
  Transfer2DUpdateGradientBox();
}


void MainWindow::Transfer2DLoad() {
  QString fileName =
    QFileDialog::getOpenFileName(this, "Load 2D Transferfunction", ".",
				 "2D Transferfunction File (*.2dt)");

  if (fileName != "") {
    m_2DTransferFunction->LoadFromFile(fileName);
  }
}


void MainWindow::Transfer2DSave() {

  QString fileName =
    QFileDialog::getSaveFileName(this, "Save 2D Transferfunction", ".",
				 "2D Transferfunction File (*.2dt)");

  if (fileName != "") {
    m_2DTransferFunction->SaveToFile(fileName);
  }
}

