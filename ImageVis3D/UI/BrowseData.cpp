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
//!    Date   : September 2008
//
//!    Copyright (C) 2008 SCI Institute

#include "BrowseData.h"

#include <QtCore/QFileInfo>
#include <ui_SettingsDlg.h>

#include "../Tuvok/LuaScripting/LuaScripting.h"

using namespace std;

BrowseData::BrowseData(MasterController& masterController, QDialog* pleaseWaitDialog, QString strDir, QWidget* parent, Qt::WindowFlags flags) :
  QDialog(parent, flags),
  m_MasterController(masterController),
  m_bDataFound(false),
  m_strDir(strDir),
  m_iSelected(0)
{
  setupUi(this);
  m_bDataFound = FillTable(pleaseWaitDialog);
}

BrowseData::~BrowseData() {
  this->m_dirInfo.clear();
}

void BrowseData::showEvent ( QShowEvent * ) {
}


void BrowseData::accept() {
  // find out which dataset is selected
  for (size_t i = 0;i < m_vRadioButtons.size();i++) {
    if (m_vRadioButtons[i]->isChecked()) {
      m_iSelected = i;
      break;
    }
  }

  QDialog::accept();
}

void BrowseData::SetBrightness(int iScale) {
  for (size_t i = 0;i < m_vRadioButtons.size();i++)
    m_vRadioButtons[i]->SetBrightness(float(iScale));
}


bool BrowseData::FillTable(QDialog* pleaseWaitDialog)
{
  shared_ptr<LuaScripting> ss(m_MasterController.LuaScript());
  m_dirInfo = ss->cexecRet<vector<shared_ptr<FileStackInfo>>>(
      "tuvok.io.scanDirectory", m_strDir.toStdWString());

  m_vRadioButtons.clear();

  for (size_t iStackID = 0;iStackID < m_dirInfo.size();iStackID++) {
    QDataRadioButton *pStackElement;
    pStackElement = new QDataRadioButton(m_dirInfo[iStackID],frame);
    pStackElement->setMinimumSize(QSize(0, 80));
    pStackElement->setChecked(iStackID==0);
    pStackElement->setIconSize(QSize(80, 80));
    verticalLayout_DICOM->addWidget(pStackElement);
    m_vRadioButtons.push_back(pStackElement);
  }

  if (pleaseWaitDialog != NULL) pleaseWaitDialog->close();

  return !m_dirInfo.empty();
}
