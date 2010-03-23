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


//!    File   : DatasetServer.cpp
//!    Author : Jens Krueger
//!             DFKI & MMCI, Saarbruecken
//!             SCI Institute, University of Utah
//!    Date   : March 2010
//
//!    Copyright (C) 2010 DFKI, MMCI, SCI Institute

#include "DatasetServer.h"

#include <stdlib.h>
#include <fstream>
#include "../Tuvok/Basics/SysTools.h"

using namespace std;

DatasetServer::DatasetServer(std::string strTempDir) : 
  m_strTempDir(strTempDir)
{
}

DatasetServer::~DatasetServer() {
  for (size_t a = 0;a< tcpServers.size();a++) {
  	if (tcpServers[a]->isListening()) tcpServers[a]->close();
    delete tcpServers[a];
    tcpServers[a] = NULL;
  } 
}

QString DatasetServer::startServers(std::vector< DatasetTFPair > &aDatasets,
                                    int startPort) {

  // no servers ... nothing todo
  if (aDatasets.size() == 0)  return "";

  // delete servers if we have too many
  for (size_t a = aDatasets.size();a< tcpServers.size();a++) {
    disconnect(tcpServers[a], SIGNAL(newConnection()), this, SLOT(sendData()));
    delete tcpServers[a];
  }

  // create new ones of we have to few
  for (size_t a = tcpServers.size();a< aDatasets.size();a++) {
    tcpServers.push_back( new QTcpServer(this) );
    connect(tcpServers[a], SIGNAL(newConnection()), this, SLOT(sendData()));
  }

  for (size_t a = 0;a<tcpServers.size();a++) {
    if (!tcpServers[a]->listen(QHostAddress::Any, quint16(startPort+a) )) {
      return tr("Unable to start the server %1 at port %2")
          .arg(tcpServers[a]->errorString()).arg(startPort+a);
    }
  }

  copy(aDatasets.begin(),aDatasets.end(),back_inserter(m_aDatasets)); 
  m_startPort = startPort;
  
  return "";
}

void DatasetServer::stopServers() {
  // stop listening
  for (size_t a = 0;a<tcpServers.size();a++) {
    if (tcpServers[a]->isListening()) tcpServers[a]->close();
  }
}

void DatasetServer::sendData()
{
  QTcpServer* tcpServer = dynamic_cast<QTcpServer *>(sender());

  if (!tcpServer) return;

  int index = tcpServer->serverPort()-m_startPort;
	std::string strFilename   = m_aDatasets[index].dataset;
  std::string strTFFilename = m_aDatasets[index].transferfunction;

  std::ifstream in(strFilename.c_str(), std::ios_base::binary);

  strFilename = SysTools::GetFilename(strFilename);

	if (!in.is_open()) return;

	in.seekg(0, std::ios::end);
	unsigned int iFileSize = (unsigned int)in.tellg();
	in.seekg(0);
	char* fileData = new char[iFileSize];
	in.read(fileData, iFileSize);
	in.close();

	std::ifstream inTF(strTFFilename.c_str(), std::ios_base::binary);
  strTFFilename = SysTools::GetFilename(strTFFilename);

	char* tfFileData = NULL;
	unsigned int iTFSize = 0;
	if (inTF.is_open()) {
		inTF.seekg(0, std::ios::end);
		iTFSize = (unsigned int)inTF.tellg();
		inTF.seekg(0);
		tfFileData = new char[iFileSize];
		inTF.read(tfFileData, iFileSize);
		inTF.close();
	}

  static unsigned int iSendMessage = 0;
 
  QTcpSocket *clientConnection = tcpServer->nextPendingConnection();
  connect(clientConnection, SIGNAL(disconnected()),
                                   clientConnection, 
                                   SLOT(deleteLater()));
  // send filename length
  unsigned int iFileLengt = int(strFilename.length());
  clientConnection->write((char*)&iFileLengt,4);
  // send filename
  clientConnection->write((char*)strFilename.c_str(),strFilename.length());
  // send filesize
  clientConnection->write((char*)&iFileSize,4);
  // send actual file
  clientConnection->write(fileData,iFileSize);
  // send transfer function filesize
  clientConnection->write((char*)&iTFSize,4);
  // send transfer function file
  if (iTFSize > 0) clientConnection->write(tfFileData,iTFSize);

  clientConnection->disconnectFromHost();

  delete [] fileData;
  delete [] tfFileData;
}
