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


//!    File   : QDataRadioButton.h
//!    Author : Jens Krueger
//!             SCI Institute
//!             University of Utah
//!    Date   : September 2008
//
//!    Copyright (C) 2008 SCI Institute

#pragma once

#ifndef QDATARADIOBUTTON_H
#define QDATARADIOBUTTON_H

#include "StdDefines.h"
#include <memory>
#include <QtWidgets/QRadioButton>
#include <../Tuvok/IO/DirectoryParser.h>

class QDataRadioButton : public QRadioButton
{
public:
  QDataRadioButton(std::shared_ptr<FileStackInfo> stack,
                   QWidget* parent=0);
  QDataRadioButton(std::shared_ptr<FileStackInfo> stack,
                   const QString &text, QWidget *parent=0);
  virtual ~QDataRadioButton() {}

  void SetBrightness(float fScale);

protected:
  unsigned int                        m_iCurrentImage;
  std::shared_ptr<FileStackInfo> m_stackInfo;
  float                               m_fScale;

  virtual void leaveEvent ( QEvent * event );
  virtual void mouseMoveEvent(QMouseEvent *event);

  void SetupInfo();
  void SetStackImage(unsigned int i, bool bForceUpdate = false);
};

#endif // QDATARADIOBUTTON_H
