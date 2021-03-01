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


//!    File   : RAWDialog.h
//!    Author : Jens Krueger
//!             SCI Institute
//!             University of Utah
//!    Date   : December 2008
//
//!    Copyright (C) 2008 SCI Institute

#ifndef RAWDIALOG_H
#define RAWDIALOG_H

#include "UI/AutoGen/ui_RAWDialog.h"
#include <string>
#include <StdDefines.h>
#include "../Tuvok/Basics/Vectors.h"

class RAWDialog : public QDialog, protected Ui_RAWDialog
{
  Q_OBJECT
  public:
    RAWDialog(const std::wstring& strFilename, uint64_t iFileSize, QWidget* parent = 0, Qt::WindowFlags flags = 0);
    virtual ~RAWDialog();

    UINT64VECTOR3 GetSize() const;
    FLOATVECTOR3 GetAspectRatio() const;
    unsigned int GetBitWidth() const;
    unsigned int GetEncoding() const;
    unsigned int GetHeaderSize() const;
    bool IsBigEndian() const;
    bool IsSigned() const;
    bool IsFloat() const;

    uint64_t ComputeExpectedSize() const;

  protected slots:
    void CheckValues();
    void ToggleEndianessDialog();
    void GuessHeaderSize();

  private:
    std::wstring m_strFilename;
    uint64_t m_iFileSize;
    void setupUi(QDialog *RAWDialog);

};

#endif // RAWDIALOG_H
