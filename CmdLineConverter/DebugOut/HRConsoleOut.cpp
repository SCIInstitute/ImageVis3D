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
  \file    HRConsoleOut.cpp
  \author    Jens Krueger
        SCI Institute
        University of Utah
  \version  1.0
  \date    August 2008
*/

#include <algorithm>
#include <cstdarg>
#include <cstring>
#include <iostream>

#include "HRConsoleOut.h"
#include "../../Tuvok/Basics/Console.h"

HRConsoleOut::HRConsoleOut() :
  m_iLengthLastMessage(0),
  m_bClearOldMessage(false)
{
}

HRConsoleOut::~HRConsoleOut() {
}

void HRConsoleOut::printf(enum DebugChannel channel, const char*,
                          const char* msg)
{
  char buff[16384];
#ifdef WIN32
  strncpy_s(buff, 16384, msg, 16384);
#else
  strncpy(buff, msg, 16384);
#endif

  if (m_bClearOldMessage) {
    // Remove any newlines from the string.
    for (size_t i=0;i<strlen(buff);i++) {
      if (buff[i] == '\n') {
        buff[i] = ' ';
      }
    }
  }

  std::cout << "\r" << buff;

  if (m_bClearOldMessage && channel == CHANNEL_MESSAGE) {
    size_t len = strlen(buff);
    // Clear the rest of the line, in case this message is shorter than the
    // last one was.
    for (size_t i=len;i<m_iLengthLastMessage;i++) {
      std::cout << " ";
    }
    m_iLengthLastMessage = len;
  } else {
    std::cout << std::endl;
    m_iLengthLastMessage = 0;
  }
  std::cout.flush();
}

void HRConsoleOut::printf(const char *s) const
{
  // Always make a new line for this version.
  std::cout << s << std::endl;
}
