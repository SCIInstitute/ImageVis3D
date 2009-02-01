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


//!    File   : BugRepDlg.cpp
//!    Author : Jens Krueger
//!             SCI Institute
//!             University of Utah
//!    Date   : January 2009
//
//!    Copyright (C) 2008 SCI Institute

#include "BugRepDlg.h"

#include "../Tuvok/Basics/SysTools.h"

#include <QtGui/QFileDialog>
#include <QtCore/QSettings>

using namespace std;

BugRepDlg::BugRepDlg(QWidget* parent, Qt::WindowFlags flags) : 
  QDialog(parent, flags)
{
  setupUi(this);
  label_Icon->setPixmap(QPixmap::fromImage(QImage(":/Resources/bug.png")));
}

BugRepDlg::~BugRepDlg(void)
{
}

void BugRepDlg::SelectFile() {
  QFileDialog::Options options;
#ifdef TUVOK_OS_APPLE
  options |= QFileDialog::DontUseNativeDialog;
#endif
  QString selectedFilter;

  QSettings settings;
  QString strLastDir = settings.value("Folders/BugReportFilename", ".").toString();

  QString fileName = QFileDialog::getOpenFileName(this,"Select File", strLastDir,
             "All Files (*.*)",&selectedFilter, options);

  if (!fileName.isEmpty()) {
    settings.setValue("Folders/CaptureFilename", QFileInfo(fileName).absoluteDir().path());
    lineEdit_file->setText(fileName);
  }
}

string BugRepDlg::GetDescription() const {
  return string(textEdit_desc->toPlainText().toAscii());
}

bool BugRepDlg::SubmitSysinfo() const {
  return checkBox_IncludeSysinfo->isChecked();
}

bool BugRepDlg::SubmitLog() const {
  return checkBox_IncludeLog->isChecked();
}

string BugRepDlg::GetUsername() const {
  return string(lineEdit_name->text().toAscii());
}

string BugRepDlg::GetUserMail() const {
  return string(lineEdit_email->text().toAscii());
}

string BugRepDlg::GetDataFilename() const {
  return string(lineEdit_file->text().toAscii());
}

void BugRepDlg::SetSubmitSysinfo(bool bSubmitSysinfo) {
  checkBox_IncludeSysinfo->setChecked(bSubmitSysinfo);
}

void BugRepDlg::SetSubmitLog(bool bSubmitLog) {
  checkBox_IncludeLog->setChecked(bSubmitLog);
}

void BugRepDlg::SetUsername(string strName) {
  lineEdit_name->setText(strName.c_str());
}

void BugRepDlg::SetUserMail(string strMail) {
  lineEdit_email->setText(strMail.c_str());
}
