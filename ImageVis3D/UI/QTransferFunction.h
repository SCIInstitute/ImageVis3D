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


//!    File   : QTransferFunction.h
//!    Author : Jens Krueger
//!             SCI Institute
//!             University of Utah
//!    Date   : July 2008
//
//!    Copyright (C) 2008 SCI Institute

#pragma once

#ifndef QTRANSFERFUNCTION_H
#define QTRANSFERFUNCTION_H

#include <QtWidgets/QWidget>
#include "../Tuvok/Controller/MasterController.h"

using namespace tuvok;

class QTransferFunction : public QWidget
{
  Q_OBJECT

public:

  enum executionMode { UNKNOWN=0, CONTINUOUS=1, ONRELEASE=2, MANUAL=4 };

public:
  QTransferFunction(MasterController& masterController, QWidget *parent=0);

  virtual void SetExecutionMode(executionMode iExecutionMode) {
    m_eExecutionMode = iExecutionMode;
  }

public slots:
  virtual void SetHistogramScale(int iScale) {
    SetHistogramScale(float(iScale));
  }

  virtual void SetHistogramScale(float fScale) {
    m_fHistfScale = fScale;
    m_bBackdropCacheUptodate = false;
    m_bHistogramChanged = true;
    update();
  }

  float GetHistogramScale() {
    return m_fHistfScale;
  }

  virtual void ApplyFunction() = 0;

protected:
  MasterController& m_MasterController;
  executionMode     m_eExecutionMode;
  float             m_fHistfScale;
  bool              m_bBackdropCacheUptodate;
  bool              m_bHistogramChanged;
};


#endif // QTRANSFERFUNCTION_H
