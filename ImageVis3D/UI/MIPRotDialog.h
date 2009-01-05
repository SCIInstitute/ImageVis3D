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


//!    File   : MIPRotDialog.h
//!    Author : Jens Krueger
//!             SCI Institute
//!             University of Utah
//!    Date   : December 2008
//
//!    Copyright (C) 2008 SCI Institute

#ifndef MIPROTDIALOG_H
#define MIPROTDIALOG_H

#include <StdDefines.h>
#include "UI/AutoGen/ui_MIPRotDialog.h"

class MIPRotDialog : public QDialog, protected Ui_MIPRotDialog
{
  Q_OBJECT
  public:
    MIPRotDialog(UINT32 iImages, bool bOrthoView, bool bStereo, QWidget* parent = 0, Qt::WindowFlags flags = 0);
    virtual ~MIPRotDialog();

    UINT32 GetNumImages();
    bool GetUseOrtho();
    bool GetUseStereo();

  protected slots:
    void UpdateDegreeLabel();

  private:
    void setupUi(QDialog *MIPRotDialog);

};

#endif // MIPROTDIALOG_H
