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


//!    File   : DatasetServerDialog.cpp
//!    Author : Jens Krueger
//!             DFKI & MMCI, Saarbruecken
//!             SCI Institute, University of Utah
//!    Date   : March 2010
//
//!    Copyright (C) 2010 DFKI, MMCI, SCI Institute

#include <cstdlib>
#include <fstream>
#if defined(__GNUC__) && defined(DETECTED_OS_LINUX)
# pragma GCC visibility push(default)
#endif
#include <QtGui/QtGui>
#include <QtNetwork/QtNetwork>
#if defined(__GNUC__) && defined(DETECTED_OS_LINUX)
# pragma GCC visibility pop
#endif

#include "DatasetServerDialog.h"
#include "../Tuvok/Basics/SysTools.h"


using namespace std;

DatasetServerDialog::DatasetServerDialog(std::string strTempDir, QWidget *parent) : 
  QDialog(parent),
  m_strTempDir(strTempDir),
  m_bRunning(false),
  server(strTempDir)
{
  QSettings settings;
  settings.beginGroup("DatasetServer");

  aDatasets.resize( settings.value("DatasetCount", 0).toUInt() );

  datasetList = new QListWidget(this);

  for (size_t b = 0;b< aDatasets.size();b++) {
    QString volume = tr("Volume%1").arg(b);
    QString transferfunction = tr("TransferFunction%1").arg(b);

    std::string strFilename(settings.value(volume, "").toString().toAscii());
    std::string strTFFilename(settings.value(transferfunction, "").toString().toAscii());
    aDatasets[b] = DatasetTFPair(strFilename, strTFFilename);

  }


  labelPort = new QLabel(this);
  labelPort->setText("TCP/IP Port");
  labelVolFile = new QLabel(this);
  labelVolFile->setText("Dataset");
  label1DTFile = new QLabel(this);
  label1DTFile->setText("Transfer Function");
  editPort = new QLineEdit(this);
  editPort->setText(settings.value("StartPort",22).toString());
  editVolFile = new QLineEdit(this);
  editVolFile->setReadOnly(true);
  editVolFile->setText("");
  edit1DTFile = new QLineEdit(this);
  edit1DTFile->setReadOnly(true);
  edit1DTFile->setText("");

  pickVolFile = new QToolButton(this);
  pickVolFile->setText("...");
  pick1DTFile = new QToolButton(this);
  pick1DTFile->setText("...");
  connect(pickVolFile, SIGNAL(clicked()), this, SLOT(selectVolFile()));
  connect(pick1DTFile, SIGNAL(clicked()), this, SLOT(select1DTFile()));

  addButton = new QPushButton(tr("Add To List"),this);
  removeButton = new QPushButton(tr("Remove From List"),this);
  connect(addButton, SIGNAL(clicked()), this, SLOT(addDataset()));
  connect(removeButton, SIGNAL(clicked()), this, SLOT(removeDataset()));

  statusLabel = new QLabel("",this);

  startButton = new QPushButton(tr("Start"),this);
  startButton->setAutoDefault(true);

  quitButton = new QPushButton(tr("Close"),this);
  quitButton->setAutoDefault(false);

  connect(quitButton, SIGNAL(clicked()), this, SLOT(close()));
  connect(startButton, SIGNAL(clicked()), this, SLOT(toggleServers()));

  QHBoxLayout *layoutPort = new QHBoxLayout();
  layoutPort->setMargin(0);
  layoutPort->addWidget(labelPort);
  layoutPort->addWidget(editPort);

  QHBoxLayout *layoutI3MFile = new QHBoxLayout();
  layoutI3MFile->setMargin(0);
  layoutI3MFile->addWidget(labelVolFile);
  layoutI3MFile->addWidget(editVolFile);
  layoutI3MFile->addWidget(pickVolFile);

  QHBoxLayout *layoutAddRemove = new QHBoxLayout();
  layoutAddRemove->setMargin(0);
  layoutAddRemove->addWidget(addButton);
  layoutAddRemove->addWidget(removeButton);

  QHBoxLayout *layout1DTFile = new QHBoxLayout();
  layout1DTFile->setMargin(0);
  layout1DTFile->addWidget(label1DTFile);
  layout1DTFile->addWidget(edit1DTFile);
  layout1DTFile->addWidget(pick1DTFile);

  QHBoxLayout *buttonLayout = new QHBoxLayout();
  buttonLayout->addStretch(1);
  buttonLayout->addWidget(startButton);
  buttonLayout->addWidget(quitButton);
  buttonLayout->addStretch(1);

  QVBoxLayout *mainLayout = new QVBoxLayout();
  mainLayout->addLayout(layoutI3MFile);
  mainLayout->addLayout(layout1DTFile);
  mainLayout->addLayout(layoutAddRemove);
  mainLayout->addWidget(datasetList);
  mainLayout->addLayout(layoutPort);
  mainLayout->addWidget(statusLabel);
  mainLayout->addLayout(buttonLayout);
  setLayout(mainLayout);

  setWindowTitle(tr("ImageVis3D Mobile Dataset Server"));
  settings.endGroup();

  updateListWidgetAndButtons();
}

DatasetServerDialog::~DatasetServerDialog() {
  QSettings settings;
  settings.beginGroup("DatasetServer");
  settings.setValue("StartPort", editPort->text().toInt());
  settings.setValue("DatasetCount", static_cast<unsigned>(aDatasets.size()));

  for (size_t b = 0;b< aDatasets.size();b++) {
    QString volume = tr("Volume%1").arg(b);
    QString transferfunction = tr("TransferFunction%1").arg(b);

    settings.setValue(volume, aDatasets[b].dataset.c_str());
    settings.setValue(transferfunction, aDatasets[b].transferfunction.c_str());
  }

  
  settings.endGroup();
}


void DatasetServerDialog::startServers() {
  // no servers ... nothing todo
  if(aDatasets.empty()) { return; }

  int port = editPort->text().toInt();

  QString errorMsg = server.startServers(aDatasets, port);

  if (errorMsg != "") {
    QMessageBox::critical(this, this->windowTitle(),errorMsg);
    return;
  }

  if (aDatasets.size() > 1) 
    statusLabel->setText(tr("Sharing %1 datasets on port %2.\n"
                            "Run the iPhone App now.")
						                .arg(aDatasets.size()).arg(port)  );
  else
    statusLabel->setText(tr("Sharing %1 dataset on port %2.\n"
                            "Run the iPhone App now.")
						                .arg(aDatasets.size()).arg(port) );
  startButton->setText(tr("Stop"));
  m_bRunning = true;
}

void DatasetServerDialog::stopServers() {
  if (!m_bRunning) return;

  server.stopServers();
  startButton->setText(tr("Restart"));
  statusLabel->setText(tr(""));

  m_bRunning = false;
}

void DatasetServerDialog::toggleServers() {
  if (m_bRunning) {
    stopServers();
  } else {
    startServers();
  }

}

void DatasetServerDialog::addDataset() {
  if (editVolFile->text() == "") return;

	std::string strFilename(editVolFile->text().toAscii());
	std::string strTFFilename(edit1DTFile->text().toAscii());
  aDatasets.push_back(DatasetTFPair(strFilename, strTFFilename));
  updateListWidgetAndButtons();

  stopServers();
}

void DatasetServerDialog::removeDataset() {
  int index = datasetList->currentRow();
  if (index >= 0 && index < int(aDatasets.size())) 
    aDatasets.erase(aDatasets.begin()+index);
  updateListWidgetAndButtons();

  stopServers();
}

void DatasetServerDialog::updateListWidgetAndButtons() {
  int index = datasetList->currentRow();
  datasetList->clear();
  QString qstr;
  for (size_t i = 0;i<aDatasets.size();i++) {
    if (aDatasets[i].transferfunction != "")
      qstr = tr("%1 / %2").arg(SysTools::GetFilename(aDatasets[i].dataset).c_str())
                                  .arg(SysTools::GetFilename(aDatasets[i].transferfunction).c_str());
    else 
      qstr = tr("%1 / none").arg(SysTools::GetFilename(aDatasets[i].dataset).c_str());
    datasetList->addItem(qstr);
  }
  if(!aDatasets.empty()) {
    datasetList->setCurrentRow(std::min<int>(index, int(aDatasets.size()-1)));
    removeButton->setEnabled(true);
    startButton->setEnabled(true);
  } else {
    removeButton->setEnabled(false);
    startButton->setEnabled(false);
  }
}

void DatasetServerDialog::select1DTFile() {
  QFileDialog::Options options;
#ifdef DETECTED_OS_APPLE
  options |= QFileDialog::DontUseNativeDialog;
#endif
  QString selectedFilter;

  QSettings settings;
  QString strLastDir = settings.value("Folders/Server1DTFilename", ".").toString();

  QString fileName = QFileDialog::getOpenFileName(this, "Select 1D Transfer function",
             strLastDir, "1D Transfer Functions (*.1dt)", &selectedFilter, options);

  if (!fileName.isEmpty()) {
    // add png as the default filetype if the user forgot to enter one
    if (SysTools::GetExt(fileName.toStdString()) == "")
      fileName = fileName + ".png";

    settings.setValue("Folders/Server1DTFilename", QFileInfo(fileName).absoluteDir().path());
    edit1DTFile->setText(fileName);
  }
}

void DatasetServerDialog::selectVolFile() {
  QFileDialog::Options options;
#ifdef DETECTED_OS_APPLE
  options |= QFileDialog::DontUseNativeDialog;
#endif
  QString selectedFilter;

  QSettings settings;
  QString strLastDir = settings.value("Folders/ServerVolFilename", ".").toString();

  QString fileName = QFileDialog::getOpenFileName(this, "Select Volume",
             strLastDir, "All Files (*.*)", &selectedFilter, options);

  if (!fileName.isEmpty()) {
    // add png as the default filetype if the user forgot to enter one
    if (SysTools::GetExt(fileName.toStdString()) == "")
      fileName = fileName + ".png";

    settings.setValue("Folders/ServerVolFilename", QFileInfo(fileName).absoluteDir().path());
    editVolFile->setText(fileName);
  }
}
