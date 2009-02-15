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


//!    File   : MergeDlg.cpp
//!    Author : Jens Krueger
//!             SCI Institute
//!             University of Utah
//!    Date   : February 2009
//
//!    Copyright (C) 2009 SCI Institute

#include "MergeDlg.h"
#include "ImageVis3D.h"
#include "PleaseWait.h"

#include <QtGui/QFileDialog>
#include <QtCore/QSettings>

#include "../Tuvok/Basics/SysTools.h"
#include "../Tuvok/IO/IOManager.h"

using namespace std;

MergeDlg::MergeDlg(MainWindow* parent, Qt::WindowFlags flags /* = 0 */) : 
  QDialog((QWidget*)parent, flags),
  m_pMainWindow(parent)
{
  setupUi();
}

MergeDlg::~MergeDlg(void)
{
  for (size_t i = 0;i<m_vDataSetList.size();i++) {
    delete m_vDataSetList[i];
  }
  m_vDataSetList.clear();
}

void MergeDlg::AnalyzeCurrentDataset() {
  int iCurrent = listWidget_datasets->currentRow();
  if (iCurrent >= 0) {
    PleaseWaitDialog pleaseWait(this);
    pleaseWait.SetText("Analyzing dataset ...");
    pleaseWait.AttachLabel(&m_pMainWindow->m_MasterController);

    RangeInfo info;
    if (m_pMainWindow->m_MasterController.IOMan()->AnalyzeDataset(m_vDataSetList[iCurrent]->m_strFilename, info)) {
      m_vDataSetList[iCurrent]->m_bAnalyzed = true;
      m_vDataSetList[iCurrent]->m_iValueType = info.m_iValueType;
      m_vDataSetList[iCurrent]->m_fRange = info.m_fRange;
      m_vDataSetList[iCurrent]->m_iRange = info.m_iRange;
      m_vDataSetList[iCurrent]->m_uiRange = info.m_uiRange;
    } else {
      m_pMainWindow->ShowWarningDialog("Analysis Error", "Unable to Analyze the dataset");
      m_vDataSetList[iCurrent]->m_bAnalyzed = false;
    }
    pleaseWait.close();
    UpdateValueFields();
  }
}

void MergeDlg::setupUi() {
  Ui_MergeDlg::setupUi(this);
  IsDatasetSelected(false);
  pushButton_save->setEnabled(m_vDataSetList.size()>0);
  UpdateValueFields();
}

void MergeDlg::IsDatasetSelected(bool bIsDatasetsSelected) {
  pushButton_removeData->setEnabled(bIsDatasetsSelected);
  groupBox_Info->setVisible(bIsDatasetsSelected);
  groupBox_SB->setVisible(bIsDatasetsSelected);
}

void MergeDlg::ChangedActiveDataset() {
  int iCurrent = listWidget_datasets->currentRow();
  UpdateValueFields();
  if (iCurrent < 0) {
    IsDatasetSelected(false);
    return;
  }
  IsDatasetSelected(true); 
}

void MergeDlg::AddDataset() {
  QFileDialog::Options options;
  #ifdef TUVOK_OS_APPLE
    options |= QFileDialog::DontUseNativeDialog;
  #endif
  QString selectedFilter;

  QSettings settings;
  QString strLastDir = settings.value("Folders/MergeInput", ".").toString();

  QString dialogString = m_pMainWindow->m_MasterController.IOMan()->GetLoadDialogString().c_str();
  QString fileName = QFileDialog::getOpenFileName(this,
               "Add Dataset", strLastDir,
               dialogString,&selectedFilter, options);

  if (!fileName.isEmpty()) {
    settings.setValue("Folders/MergeInput", QFileInfo(fileName).absoluteDir().path());

    DataSetListElem* l = new DataSetListElem(string(fileName.toAscii()));
    m_vDataSetList.push_back(l);
    UpadeListView();
    listWidget_datasets->setCurrentRow(int(m_vDataSetList.size()-1));
  }

  pushButton_save->setEnabled(true);
}

void MergeDlg::RemoveDataset() {
  int iCurrent = listWidget_datasets->currentRow();
  if (iCurrent < 0) return;
  m_vDataSetList.erase(m_vDataSetList.begin()+iCurrent);
  UpadeListView();

  pushButton_save->setEnabled(m_vDataSetList.size()>0);
}

void MergeDlg::ExecuteMerge() {
  accept();
}

void MergeDlg::CancelMerge() {
  reject();
}

void MergeDlg::UpadeListView() {
  int iCurrent = listWidget_datasets->currentRow();
  listWidget_datasets->clear();

  for (size_t i = 0;i<m_vDataSetList.size();i++) {
    QString strDesc = m_vDataSetList[i]->m_strDisplayName.c_str();
    listWidget_datasets->addItem ( strDesc );
  }

  if (m_vDataSetList.size() > 0 && iCurrent >= 0) {
    listWidget_datasets->setCurrentRow(std::min<int>(iCurrent, int(m_vDataSetList.size())-1));
  }

  IsDatasetSelected(listWidget_datasets->currentRow() >= 0);
}

void MergeDlg::UpdateValueFields() {
  int iCurrent = listWidget_datasets->currentRow();
  if (iCurrent <0) return;

  if (m_vDataSetList[iCurrent]->m_bAnalyzed) {
    QString strDesc;
    switch (m_vDataSetList[iCurrent]->m_iValueType) {
      case 0 : strDesc = tr("The data set contains floting point data in the range %1 to %2").arg(m_vDataSetList[iCurrent]->m_fRange.first).arg(m_vDataSetList[iCurrent]->m_fRange.second);
               break;
      case 1 : strDesc = tr("The data set contains integer data in the range %1 to %2").arg(m_vDataSetList[iCurrent]->m_iRange.first).arg(m_vDataSetList[iCurrent]->m_iRange.second);
               break;
      case 2 : strDesc = tr("The data set contains integer data in the range %1 to %2").arg(m_vDataSetList[iCurrent]->m_uiRange.first).arg(m_vDataSetList[iCurrent]->m_uiRange.second);
               break;
      default : strDesc = "The dataset description is invalid.";
              break;
    }
    label_Info->setText(strDesc);
  } else {
    label_Info->setText("Click Analyze to get information about the data set");
  }

  doubleSpinBox_bias->setValue(m_vDataSetList[iCurrent]->m_fBias);
  doubleSpinBox_scale->setValue(m_vDataSetList[iCurrent]->m_fScale);
}


void MergeDlg::ChangedScale(double fScale) {
  int iCurrent = listWidget_datasets->currentRow();
  if (iCurrent <0) return;

  m_vDataSetList[iCurrent]->m_fScale = fScale;
}

void MergeDlg::ChangedBias(double fBias) {
  int iCurrent = listWidget_datasets->currentRow();
  if (iCurrent <0) return;

  m_vDataSetList[iCurrent]->m_fBias = fBias;
}
