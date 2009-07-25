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

//!    File   : I3MDialog.cpp
//!    Author : Jens Krueger
//!             DFKI & MMCI, Saarbruecken
//!             SCI Institute, University of Utah
//!    Date   : July 2009
//
//!    Copyright (C) 2009 DFKI, MMCI, SCI Institute

#include "I3MDialog.h"
#include <QtCore/QSettings>
#include <QtNetwork/QTcpSocket>
#include <QtGui/QMessageBox>
#include <QtGui/QInputDialog>

#include <fstream>
#include "../Tuvok/Basics/SysTools.h"
#include "../Tuvok/IO/IOManager.h"
#include "../Tuvok/IO/Metadata.h"
#include "DebugOut/QTLabelOut.h"

using namespace tuvok;

I3MDialog::I3MDialog(MasterController* pMasterController, const UVFDataset* currentDataset, const std::string& strTmpDir, QWidget* parent, Qt::WindowFlags flags) : 
  QDialog(parent, flags),
  m_tcpServer(NULL),
  m_iPort(22),
  m_currentDataset(currentDataset),
  m_bDataNotConverted(true),
  m_strI3MFilename(""),
  m_str1DTFilename(""),
  m_strSource1DTFilename(""),
  m_strTempDir(strTmpDir),
  m_pMasterController(pMasterController)
{
  // the usual UI setup
  setupUi(this);

  // read port from settings
  QSettings settings;
  m_iPort = settings.value("Network/I3MPort", m_iPort).toUInt();

  // create port object and setup callback
  m_tcpServer = new QTcpServer(this);
  connect(m_tcpServer, SIGNAL(newConnection()), this, SLOT(SendData()));

  // find transferfunction and update big label
  std::string filenameOnly = SysTools::GetFilename(m_currentDataset->Filename());
  std::string potentialTFFile = SysTools::ChangeExt(m_currentDataset->Filename(),"1dt");

  m_strI3MFilename = m_strTempDir+SysTools::ChangeExt(filenameOnly,"i3m");


  QString qs;
  if (SysTools::FileExists(potentialTFFile)) {
    qs = tr("Dataset: %1\nTransferFunction: %2").arg(filenameOnly.c_str()).arg(SysTools::ChangeExt(filenameOnly,"1dt").c_str());
    m_strSource1DTFilename = potentialTFFile;
    m_str1DTFilename = SysTools::ChangeExt(m_strI3MFilename,"1dt");
  } else {
    qs = tr("Dataset: %1").arg(filenameOnly.c_str());
    m_strSource1DTFilename = "";
    m_str1DTFilename = "";
  }

  label_files->setText(qs);
}

I3MDialog::~I3MDialog(void)
{
  // if port is open close it and then delete it
  if (m_tcpServer->isListening()) m_tcpServer->close();
  delete m_tcpServer;
  if (m_bDataNotConverted) CleanupTemp();
}

void I3MDialog::Start()
{
  // if we are already listening close the port
  if (m_tcpServer->isListening()) m_tcpServer->close();

  // convert UVF to i3m and prepare TF (rescaling if necessary)
  if (m_bDataNotConverted) 
    if (!ConvertData()) return;

  // open port and check if everything is ok
  if (!m_tcpServer->listen(QHostAddress::Any, m_iPort)) {
    QMessageBox::critical(this, this->windowTitle(),
                          tr("Unable to start the server: %1.")
                          .arg(m_tcpServer->errorString()));
    return;
  }

  // update status
  label_Status->setText(tr("The server is running on port %1.\n"
                          "Connect ImageVis3D Mobile to this system now.")
					               .arg(m_tcpServer->serverPort())  );
}

void I3MDialog::SelectPort()
{
  // open dialog and ask for a port
  bool bOk;
  int iNewPort = QInputDialog::getInteger(this,
                                      tr("TCP Port"),
                                      tr("Select the TCP Port for this server to run on:"), m_iPort, 1, 65535, 1, &bOk);
  
  // if the user hit the OK button ... 
  if (bOk) {
    // ... update member variable
    m_iPort = iNewPort;
    // ... update settings
    QSettings settings;
    settings.setValue("Network/I3MPort", m_iPort);
    // ... restart server if it is already running
    if (m_tcpServer->isListening()) Start();
  }
}

void I3MDialog::SendData()
{
  // open volume
	std::ifstream in(m_strI3MFilename.c_str(), std::ios_base::binary);
  if (!in.is_open()) {
    QMessageBox::critical(this, this->windowTitle(),
                          tr("Unable to open converted volume: %1.")
                          .arg(m_strI3MFilename.c_str()));
    return;
  }
	in.seekg(0, std::ios::end);
	unsigned int iFileSize = (unsigned int)in.tellg();
	in.seekg(0);
	char* fileData = new char[iFileSize];
	in.read(fileData, iFileSize);
	in.close();

  // open transfer function
  unsigned int iTFSize = 0;
  char* tfFileData = NULL;
  if (m_str1DTFilename != "") {
    std::ifstream inTF(m_str1DTFilename.c_str(), std::ios_base::binary);
	  if (inTF.is_open()) {
		  inTF.seekg(0, std::ios::end);
		  iTFSize = (unsigned int)inTF.tellg();
		  inTF.seekg(0);
		  tfFileData = new char[iFileSize];
		  inTF.read(tfFileData, iFileSize);
		  inTF.close();
	  }
  }

  // update label to notify the user about the upcomming send operation
	static unsigned int iSendMessage = 0;
  label_Status->setText(tr("The server is running on port %1.\n"
                          "Connected! Sending dataset to device %2.")
					  .arg(m_tcpServer->serverPort()).arg(++iSendMessage)  );

  // open socket
  QTcpSocket *clientConnection = m_tcpServer->nextPendingConnection();
  connect(clientConnection, SIGNAL(disconnected()), clientConnection, SLOT(deleteLater()));
	// send filename length
  std::string strFilenameOnly = SysTools::GetFilename(m_strI3MFilename);
	unsigned int iFilenameLength = int(strFilenameOnly.length());
  clientConnection->write((char*)&iFilenameLength,4);
	// send filename
  clientConnection->write((char*)strFilenameOnly.c_str(),iFilenameLength);
	// send filesize
	clientConnection->write((char*)&iFileSize,4);
	// send actual file
	clientConnection->write(fileData,iFileSize);
	// send transfer function filesize
	clientConnection->write((char*)&iTFSize,4);
	// send transfer function file
	if (iTFSize > 0) clientConnection->write(tfFileData,iTFSize);
  // disconnect
	clientConnection->disconnectFromHost();

  // update label
	if (iSendMessage == 1) {
		label_Status->setText(tr("The server is running on port %1.\n"
								"Dataset send to mobile device.")
								.arg(m_tcpServer->serverPort()) );
	} else {
		label_Status->setText(tr("The server is running on port %1.\n"
								"Dataset send to %2 mobile devices.")
								.arg(m_tcpServer->serverPort()).arg(iSendMessage)  );
	}

  // delete arrays
	delete [] fileData;
	delete [] tfFileData;
}


bool I3MDialog::ConvertData() {
  QString labelText = label_files->text();
  label_files->setText("Converting "+labelText);
  

  // UVF to I3M

  // first, find the smalest LOD with every dimension larger or equal to 128 (if possible)
  int iLODLevel = int(m_currentDataset->GetInfo().GetLODLevelCount())-1;
  for (;iLODLevel>=0;iLODLevel--) {
    UINTVECTOR3 vLODSize = UINTVECTOR3(m_currentDataset->GetInfo().GetDomainSize(iLODLevel));
    if (vLODSize.x >= 128 &&
        vLODSize.y >= 128 &&
        vLODSize.z >= 128) break;
  }

  // now convert it
  QTLabelOut* pLabelOut = new QTLabelOut(label_Status, this);
  pLabelOut->SetOutput(true, true, true, false);
  m_pMasterController->AddDebugOut(pLabelOut);

  if (!m_pMasterController->IOMan()->ExportDataset(m_currentDataset, iLODLevel, m_strI3MFilename, m_strTempDir)) {
		label_Status->setText(tr("Failed to convert the current dataset into the i3m format for the mobile device."));
    label_files->setText(labelText);  
    return false;
  }
  label_Status->setText(tr("Converting transfer function."));
  label_Status->update();

  m_pMasterController->RemoveDebugOut(pLabelOut);

  // resample 1D tf to 8bit
  if (m_strSource1DTFilename != "") {
    TransferFunction1D tfIn(m_strSource1DTFilename);
    tfIn.Resample(256);
    tfIn.Save(m_str1DTFilename);
  }

  label_Status->setText(tr("Conversion complete!"));
  label_Status->update();

  m_bDataNotConverted = false;
  return true;
}

void I3MDialog::CleanupTemp() {
  if (SysTools::FileExists(m_strI3MFilename)) remove(m_strI3MFilename.c_str());
  if (SysTools::FileExists(m_str1DTFilename)) remove(m_str1DTFilename.c_str());
}

