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


//!    File   : MetadataDlg.cpp
//!    Author : Jens Krueger
//!             SCI Institute
//!             University of Utah
//!    Date   : January 2009
//
//!    Copyright (C) 2008 SCI Institute

#include "MetadataDlg.h"

MetadataDlg::MetadataDlg(QWidget* parent, Qt::WindowFlags flags /* = 0 */) :
  QDialog(parent, flags)
{
  setupUi(this);
}

MetadataDlg::~MetadataDlg(void)
{
}

void MetadataDlg::SetFilename(const QString& strFilename) {
  TextLabel_Metadata->setText("Metadata for " + strFilename);
}

void MetadataDlg::SetMetadata(const std::vector<std::pair<std::wstring, std::wstring>>& metadata) {
  listWidget_metadata->clear();

  for (size_t i = 0;i<metadata.size();i++) {
    QString s = QString::fromStdWString(metadata[i].first) + " = " + QString::fromStdWString(metadata[i].second);
    listWidget_metadata->addItem(s);
  }
}
