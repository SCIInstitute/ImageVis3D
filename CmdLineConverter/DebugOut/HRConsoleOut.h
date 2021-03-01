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
  \file    HRConsoleOut.h
  \author    Jens Krueger
        SCI Institute
        University of Utah
  \version  1.0
  \date    August 2008
*/


#pragma once

#ifndef HRCONSOLEOUT_H
#define HRCONSOLEOUT_H

#include "../../Tuvok/DebugOut/AbstrDebugOut.h"

class HRConsoleOut : public AbstrDebugOut{
  public:
    HRConsoleOut();
    ~HRConsoleOut();

    void SetClearOldMessage(bool bClearOldMessage) {m_bClearOldMessage = bClearOldMessage;}
    bool GetClearOldMessage() {return m_bClearOldMessage;}

    virtual void printf(enum DebugChannel, const char* source,
                        const char* msg);
    virtual void printf(const char *s) const;

  private:
    size_t m_iLengthLastMessage;
    bool   m_bClearOldMessage;
};

#endif // HRCONSOLEOUT_H
