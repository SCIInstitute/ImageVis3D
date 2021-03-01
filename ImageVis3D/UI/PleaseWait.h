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


//!    File   : PleaseWait.h
//!    Author : Jens Krueger
//!             SCI Institute
//!             University of Utah
//!    Date   : July 2008
//
//!    Copyright (C) 2008 SCI Institute

#ifndef PLEASEWAIT_H
#define PLEASEWAIT_H

#include <ui_PleaseWait.h>
#include "DebugOut/QTLabelOut.h"
#include "../Tuvok/Controller/MasterController.h"

using namespace tuvok;
class QPushButton;

class PleaseWaitDialog : public QDialog, protected Ui_PleaseWaitDialog
{
  Q_OBJECT
  public:
    PleaseWaitDialog(QWidget* parent,
                     Qt::WindowFlags flags = Qt::Tool,
                     bool bHasCancelButton= false);
    virtual ~PleaseWaitDialog();

    QTLabelOut* AttachLabel(MasterController* pMasterController);
    void DetachLabel();

    void SetText(QString text) {
      label->setText(text);
      if(this->isHidden()) {
        this->show();
      }
    }
    QLabel* GetStatusLabel() const {return label_Status;}

    void closeEvent(QCloseEvent *event);

    bool Canceled() const { return m_bCanceled; }
  private slots:
    void CancelClicked();

  signals:
    void CancelSignal();

  protected:
    MasterController* m_pMasterController;
    QTLabelOut*       m_pLabelOut;
    QPushButton*      m_ButtonCancel;
    bool              m_bCanceled;
};

#endif // PLEASEWAIT_H
