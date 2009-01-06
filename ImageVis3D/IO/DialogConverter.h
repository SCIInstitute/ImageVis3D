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

/**
  \file    DialogConverter.h
  \author    Jens Krueger
        SCI Institute
        University of Utah
  \version  1.0
  \date    December 2008
*/


#pragma once

#ifndef DIALOGCONVERTER_H
#define DIALOGCONVERTER_H

#include "../Tuvok/IO/RAWConverter.h"
#include <QtGui/QWidget>

class MasterController;

class DialogConverter : public RAWConverter {
public:
  DialogConverter(QWidget* parent);
  virtual ~DialogConverter() {}

  virtual bool Convert(const std::string& strSourceFilename, const std::string& strTargetFilename, const std::string& strTempDir, MasterController* pMasterController, bool bNoUserInteraction);

protected:
  QWidget* m_parent;

};

#endif // DIALOGCONVERTER_H
