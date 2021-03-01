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


//!    File   : FTPDialog.cpp
//!    Author : Jens Krueger
//!             SCI Institute
//!             University of Utah
//!    Date   : December 2008
//
//!    Copyright (C) 2008 SCI Institute

#include "FTPDialog.h"
#include "IO/3rdParty/QFtp/qftp.h"
#include <QtCore/QUrl>
#include <QtCore/QFile>

#include "../Tuvok/Basics/SysTools.h"
#include "../Tuvok/Controller/Controller.h"

using namespace std;

FTPDialog::FTPDialog(const wstring& strSource, const wstring& strTargetServer,
                     const wstring& strTargetPath, QWidget* parent /* = 0 */,
                     Qt::WindowFlags flags /* = 0 */)
  : QDialog(parent, flags)
  ,m_strSource(strSource)
  ,m_strTargetServer(strTargetServer)
  ,m_strTargetPath(strTargetPath)
  ,m_pFtp(NULL)
{
  setupUi(this);
}

FTPDialog::~FTPDialog(void)
{
  Disconnect();
}

void FTPDialog::Start() {
  show();

  m_pFtp = new QFtp(this);
  connect(m_pFtp, SIGNAL(commandFinished(int, bool)), this,
                  SLOT(ftpCommandFinished(int, bool)));
  connect(m_pFtp, SIGNAL(dataTransferProgress(qint64, qint64)), this,
                  SLOT(updateDataTransferProgress(qint64, qint64)));
  connect(m_pFtp, SIGNAL(done(bool)), this, SLOT(finished(bool)));

  label_TransferDesc->setText(tr("Connecting to %1...").arg(SysTools::toNarrow(m_strTargetServer).c_str()));

  QUrl url(SysTools::toNarrow(m_strTargetServer).c_str());
  if (!url.isValid() || url.scheme().toLower() != QLatin1String("ftp")) {
    m_pFtp->connectToHost(SysTools::toNarrow(m_strTargetServer).c_str(), 21);
    m_pFtp->login();
  } else {
    m_pFtp->connectToHost(url.host(), url.port(21));

    if (!url.userName().isEmpty()) {
      m_pFtp->login(QUrl::fromPercentEncoding(url.userName().toLatin1()),
                    url.password());
    } else {
      m_pFtp->login();
    }
    if (!url.path().isEmpty()) {
      m_pFtp->cd(url.path());
    }
    
  }

  m_pFile = new QFile(SysTools::toNarrow(m_strSource).c_str());
  if (!m_pFile->open(QIODevice::ReadOnly)) {
    T_ERROR("Could not read '%s' file.", SysTools::toNarrow(m_strSource).c_str());
    delete m_pFile;
    m_pFile = NULL;
    emit TransferFailure();
    close();
    return;
  }

  MESSAGE("putting '%s' to '%s'", SysTools::toNarrow( m_strSource).c_str(),
          SysTools::toNarrow(m_strTargetPath).c_str());
  m_pFtp->put(m_pFile, SysTools::toNarrow(m_strTargetPath).c_str());

}


void FTPDialog::ftpCommandFinished(int cmdId, bool error) {
  if (cmdId == QFtp::ConnectToHost) {
    if (error) {
      T_ERROR("Error connecting to host: %s",
              m_pFtp->errorString().toStdString().c_str());
      m_pFile->close();
      delete m_pFile;
      m_pFile = NULL;
      emit TransferFailure();
      close();
      return;
    }
    label_TransferDesc->setText(tr("Uploading %1...").arg(SysTools::toNarrow(SysTools::GetFilename(m_strSource)).c_str()));
    return;
  }
}

void FTPDialog::updateDataTransferProgress(qint64 readBytes, qint64 totalBytes) {
  progressBar->setMaximum(totalBytes);
  progressBar->setValue(readBytes);
}

void FTPDialog::Disconnect()
{
  if (m_pFtp) {
    m_pFtp->abort();
    m_pFtp->deleteLater();
    m_pFtp = NULL;
  }
}

void FTPDialog::AbortTransfer()
{
  m_pFile->close();
  delete m_pFile;
  m_pFile = NULL;
  Disconnect();
  emit TransferFailure();
  close();
}

void FTPDialog::finished(bool error)
{
  if (error) {
    T_ERROR("Error putting data on the remote host.");
    m_pFile->close();
    delete m_pFile;
    m_pFile = NULL;
    emit TransferFailure();
    close();
    return;
  } else {
    MESSAGE("File transfer complete.");
    m_pFile->close();
    delete m_pFile;
    m_pFile = NULL;
    emit TransferSuccess();
    close();
    return;
  }
}
