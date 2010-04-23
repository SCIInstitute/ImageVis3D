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


//!    File   : DatasetServer.h
//!    Author : Jens Krueger
//!             DFKI & MMCI, Saarbruecken
//!             SCI Institute, University of Utah
//!    Date   : March 2010
//
//!    Copyright (C) 2010 DFKI, MMCI, SCI Institute

#pragma once

#ifndef DATASETSERVER_H
#define DATASETSERVER_H

#include <vector>
#include <string>
#include <QtNetwork/QtNetwork>

class DatasetTFPair {
public:
  DatasetTFPair() :
    dataset(""),
    transferfunction("") {}
  DatasetTFPair(const std::string& d, const std::string& t) :
    dataset(d),
    transferfunction(t) {}
  std::string dataset;
  std::string transferfunction;
};

class DatasetServer : QObject {
    Q_OBJECT

public:
  DatasetServer(std::string strTempDir);
  ~DatasetServer();

  QString startServers(std::vector< DatasetTFPair > &aDatasets, int startPort);
  void stopServers();

private slots:
  void sendData();

private:
  int m_startPort;
  std::vector< QTcpServer * > tcpServers;
  std::vector< DatasetTFPair > m_aDatasets;
  std::string m_strTempDir;

};

#endif //DATASETSERVER_H
