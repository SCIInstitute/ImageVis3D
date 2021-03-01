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


//!    File   : FTPDialog.h
//!    Author : Jens Krueger
//!             SCI Institute
//!             University of Utah
//!    Date   : December 2008
//
//!    Copyright (C) 2008 SCI Institute

#ifndef FTPDIALOG_H
#define FTPDIALOG_H

#include "UI/AutoGen/ui_FTPDialog.h"
#include <string>
#include <StdDefines.h>
#include "../Tuvok/Basics/Vectors.h"

class QFtp;
class QUrlInfo;
class QFile;

class FTPDialog : public QDialog, protected Ui_FTPDialog
{
  Q_OBJECT
  public:
    FTPDialog(const std::wstring& strSource, const std::wstring& strTargetServer, const std::wstring& strTargetPath, QWidget* parent = 0, Qt::WindowFlags flags = 0);
    virtual ~FTPDialog();

    void Start();

  protected slots:
     void ftpCommandFinished(int commandId, bool error);
     void updateDataTransferProgress(qint64 readBytes, qint64 totalBytes);
     void AbortTransfer();
     void finished(bool);

  signals:
     void TransferFailure();
     void TransferSuccess();

  private:
    std::wstring  m_strSource;
    std::wstring  m_strTargetServer;
    std::wstring  m_strTargetPath;
    QFtp*         m_pFtp;
    QFile*        m_pFile;

    void Disconnect();
};

#endif // FTPDIALOG_H
