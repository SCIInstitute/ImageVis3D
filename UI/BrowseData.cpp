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


//!    File   : BrowseData.cpp
//!    Author : Jens Krueger
//!             SCI Institute
//!             University of Utah
//!    Date   : July 2008
//
//!    Copyright (C) 2008 SCI Institute

#include "BrowseData.h"

#include "QDataRadioButton.h"
#include <QtCore/QFileInfo>
#include <QtGui/QMessageBox>

using namespace std;

BrowseData::BrowseData(MasterController& masterController, QDialog* pleaseWaitDialog, QString strDir, QWidget* parent, Qt::WindowFlags flags) : 
  QDialog(parent, flags),
  m_MasterController(masterController),
  m_bDataFound(false),
  m_strDir(strDir),
  m_strFilename("")
{
  setupUi(this);

  m_bDataFound = FillTable(pleaseWaitDialog);

  // TODO: add actions to set the correct filename
  m_strFilename = "DICOM Dataset";
}


void BrowseData::showEvent ( QShowEvent * ) {
}

bool BrowseData::FillTable(QDialog* pleaseWaitDialog)
{
  vector<FileStackInfo*> dirInfo = m_MasterController.IOMan()->ScanDirectory(m_strDir.toStdString());

  for (unsigned int iStackID = 0;iStackID < dirInfo.size();iStackID++) {
    QDataRadioButton *pDICOMElement;
    pDICOMElement = new QDataRadioButton(dirInfo[iStackID],frame);
    pDICOMElement->setMinimumSize(QSize(0, 80));
    pDICOMElement->setChecked(iStackID==0);
    pDICOMElement->setIconSize(QSize(80, 80));
    verticalLayout_DICOM->addWidget(pDICOMElement);
  }

  if (pleaseWaitDialog != NULL) pleaseWaitDialog->close();

  return dirInfo.size() > 0;
}

