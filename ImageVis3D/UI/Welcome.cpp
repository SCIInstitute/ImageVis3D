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


//!    File   : Welcome.cpp
//!    Author : Jens Krueger
//!             SCI Institute
//!             University of Utah
//!    Date   : January 2009
//
//!    Copyright (C) 2008 SCI Institute

#include "Welcome.h"

WelcomeDialog::WelcomeDialog(QWidget* parent, Qt::WindowFlags flags /* = 0 */) :
  QDialog(parent, flags)
{
  setupUi(this);
  label_logo->setPixmap(QPixmap::fromImage(QImage(":/Resources/imagevis3dmini.png")));
}

WelcomeDialog::~WelcomeDialog(void)
{
}

void WelcomeDialog::CheckUpdates()
{

  emit CheckUpdatesClicked();
}

void WelcomeDialog::OnlineVideoTut()
{

  emit OnlineVideoTutClicked();
}

void WelcomeDialog::OnlineHelp()
{
  emit OnlineHelpClicked();
}

void WelcomeDialog::GetExampleData()
{
  emit GetExampleDataClicked();
}

void WelcomeDialog::OpenManual()
{
  emit OpenManualClicked();
}

void WelcomeDialog::OpenFromFile()
{
  accept();
  emit OpenFromFileClicked();
}

void WelcomeDialog::OpenFromDir()
{
  accept();
  emit OpenFromDirClicked();
}

void WelcomeDialog::OpenMRU()
{
  MRUButton* button = (MRUButton*)qobject_cast<QPushButton *>(sender());
  accept();

  emit OpenFromFileClicked(button->strFilename);
}


void WelcomeDialog::ClearMRUItems() {
  for (size_t i = 0;i<m_vMRUItems.size();i++) {
    verticalLayout_OpenFile->removeWidget(m_vMRUItems[i]);
    disconnect(m_vMRUItems[i], SIGNAL(clicked()), this, SLOT(OpenMRU()));
    delete m_vMRUItems[i];
  }

  m_vMRUItems.clear();
}

void WelcomeDialog::AddMRUItem(std::wstring strDesc, std::wstring strFilename) {
  MRUButton* b = new MRUButton(groupBox_OpenFile);
  b->setText(QString::fromStdWString(strDesc));
  b->setToolTip(QString::fromStdWString(strFilename));
  b->setMinimumHeight(23);  // to make buttons work on the MAC
  b->strFilename = strFilename;

  connect(b, SIGNAL(clicked()), this, SLOT(OpenMRU()));
  verticalLayout_OpenFile->addWidget(b);
  m_vMRUItems.push_back(b);
}
