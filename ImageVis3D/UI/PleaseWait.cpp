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


//!    File   : PleaseWait.cpp
//!    Author : Jens Krueger
//!             SCI Institute
//!             University of Utah
//!    Date   : July 2008
//
//!    Copyright (C) 2008 SCI Institute

#include "PleaseWait.h"
#include <QtWidgets/QPushButton>

PleaseWaitDialog::PleaseWaitDialog(QWidget* parent, Qt::WindowFlags flags,
                                   bool bHasCancelButton) :
  QDialog(parent, flags),
  m_pMasterController(NULL),
  m_pLabelOut(NULL),
  m_ButtonCancel(NULL),
  m_bCanceled(false)
{
  setupUi(this);

  if (bHasCancelButton) {
    m_ButtonCancel = new QPushButton(this);
    m_ButtonCancel->setText(QString::fromUtf8("Cancel"));
    verticalLayout->addWidget(m_ButtonCancel);

    QObject::connect(m_ButtonCancel, SIGNAL(clicked()), this,
                     SLOT(CancelClicked()));
  }
}

PleaseWaitDialog::~PleaseWaitDialog(void)
{
  DetachLabel();
  delete m_ButtonCancel;
}

void PleaseWaitDialog::CancelClicked()
{
  m_ButtonCancel->setText(QString::fromUtf8("Cancelling ..."));
  m_ButtonCancel->setEnabled(false);
  m_ButtonCancel->update();
  m_bCanceled = true;
  emit Canceled();
}

QTLabelOut* PleaseWaitDialog::AttachLabel(MasterController* pMasterController) {
  if (m_pLabelOut) return m_pLabelOut;

  m_pMasterController = pMasterController;

  m_pLabelOut = new QTLabelOut(label_Status, this);
  m_pLabelOut->SetOutput(true, true, true, false);
  m_pMasterController->AddDebugOut(m_pLabelOut);

  return m_pLabelOut;
}

void PleaseWaitDialog::DetachLabel() {
  if(m_pMasterController) {
    m_pMasterController->RemoveDebugOut(m_pLabelOut);
  }
  m_pLabelOut = NULL;
}

void PleaseWaitDialog::closeEvent(QCloseEvent *)
{
  DetachLabel();
}
