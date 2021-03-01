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
  \file    QTOut.cpp
  \author    Jens Krueger
        SCI Institute
        University of Utah
  \version  1.0
  \date    September 2008
*/

#include "QTOut.h"
#include <stdarg.h>
#include <stdio.h>
#include <sstream>

using namespace std;

#ifdef WIN32
  #include <windows.h>
  // undef stupid windows defines to max and min
  #ifdef max
  #undef max
  #endif

  #ifdef min
  #undef min
  #endif
#endif

QTOut::QTOut(QListWidget *listWidget) :
  m_listWidget(listWidget)
{
  Message("QTOut::QTOut","Starting up QTListviewDebug out");
}

QTOut::~QTOut() {
  Message("QTOut::~QTOut","Shutting down QTListviewDebug out");
}

void QTOut::printf(enum DebugChannel channel, const char* source,
                   const char* buff)
{
  std::ostringstream msg;
  msg << ChannelToString(channel) << " (" << source << "): " << buff;

  m_listWidget->addItem(msg.str().c_str());

  if(static_cast<size_t>(channel) < m_bRecordLists.size() &&
     m_bRecordLists[channel]) {
    m_strLists[channel].push_back(msg.str());
  }
}

void QTOut::printf(const char *s) const
{
  m_listWidget->addItem(s);

  // Can't record on a channel.. we don't know what channel this is!
}
