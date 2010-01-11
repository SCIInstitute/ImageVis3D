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

//!    File   : I3MDialog.h
//!    Author : Jens Krueger
//!             DFKI & MMCI, Saarbruecken
//!             SCI Institute, University of Utah
//!    Date   : July 2009
//
//!    Copyright (C) 2009 DFKI, MMCI, SCI Institute

#ifndef I3MDIALOG_H
#define I3MDIALOG_H

#include "AutoGen/ui_I3MDialog.h"
#include <StdDefines.h>
#include <QtNetwork/QTcpServer>
#include "../Tuvok/IO/uvfDataset.h"
#include "../Tuvok/IO/uvfDataset.h"
#include "../Tuvok/Controller/Controller.h"

class I3MDialog : public QDialog, protected Ui_I3MDialog
{
  Q_OBJECT

  public:
    I3MDialog(const tuvok::UVFDataset* currentDataset, const std::string& strTmpDir, QWidget* parent, Qt::WindowFlags flags = Qt::Dialog);
    virtual ~I3MDialog();

  protected slots:
    virtual void SendData();
    virtual void Start();
    virtual void SetPort();
    virtual void SetVersion();

  protected:
    UINT32         m_iSendMessage;
    QTcpServer*    m_tcpServer;
    UINT32         m_iPort;
    UINT32         m_iI3MVersion;
    const tuvok::UVFDataset* m_currentDataset;
    bool           m_bDataNotConverted;
    bool           m_bCreatedSharedData;
    std::string    m_strSharedDataFilename;
    bool           m_bCreatedTF;
    std::string    m_strSharedTFFilename;
    std::string    m_strSource1DTFilename;
    std::string    m_strTempDir;
    MasterController* m_pMasterController;

    bool ConvertData();
    bool ConvertDataV1();
    bool ConvertDataV2();
    void CleanupTemp();
};

#endif // I3MDIALOG_H
