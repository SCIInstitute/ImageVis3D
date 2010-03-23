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


//!    File   : DatasetServerDialog.h
//!    Author : Jens Krueger
//!             DFKI & MMCI, Saarbruecken
//!             SCI Institute, University of Utah
//!    Date   : March 2010
//
//!    Copyright (C) 2010 DFKI, MMCI, SCI Institute

#pragma once

#ifndef DATASETSERVERDIALOG_H
#define DATASETSERVERDIALOG_H

#include <QtGui/QDialog>

QT_BEGIN_NAMESPACE
class QLineEdit;
class QLabel;
class QPushButton;
class QTcpServer;
class QListWidget;
class QToolButton;
QT_END_NAMESPACE

#include <vector>
#include <string>
#include <DatasetServer/DatasetServer.h>

class DatasetServerDialog : public QDialog
{
    Q_OBJECT

public:
  DatasetServerDialog(std::string strTempDir, QWidget *parent = 0);
  ~DatasetServerDialog();

private slots:
  void toggleServers();
  void addDataset();
  void removeDataset();
  void select1DTFile();
  void selectVolFile();

private:
  QListWidget* datasetList;
  QLabel *statusLabel;
  QPushButton *startButton;
  QPushButton *quitButton;
  QLabel *labelPort;
  QLabel *labelVolFile;
  QLabel *label1DTFile;
  QLineEdit *editPort;
  QLineEdit *editVolFile;
  QLineEdit *edit1DTFile;
  QPushButton *addButton;
  QPushButton *removeButton;
  QToolButton *pickVolFile;
  QToolButton *pick1DTFile;

  std::vector< DatasetTFPair > aDatasets;
  std::string m_strTempDir;
  bool m_bRunning;
  DatasetServer server;

  void updateListWidgetAndButtons();
	void startServers();
	void stopServers();

};

#endif //DATASETSERVERDIALOG_H
