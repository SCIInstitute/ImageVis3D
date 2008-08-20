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


//!    File   : main.cpp
//!    Author : Jens Krueger
//!             SCI Institute
//!             University of Utah
//!    Date   : July 2008
//
//!    Copyright (C) 2008 SCI Institute

#include <QtGui/QApplication>
#include <UI/ImageVis3D.h>
#include <Controller/MasterController.h>

#include <Basics/SysTools.h>
#include <DebugOut/TextfileOut.h>

int main(int argc, char* argv[])
{
  SysTools::CmdLineParams parameters(argc, argv);

  std::string strLogFileName;
  bool bUseLogFile = parameters.GetValue("LOG",strLogFileName);
  int iLogLevel = 0;
  parameters.GetValue("LOGLEVEL",iLogLevel);

  MasterController masterController;

  QApplication app( argc, argv );
  MainWindow mainWindow(masterController, 0, Qt::Window);
  
  if (bUseLogFile) {
    TextfileOut* textout = new TextfileOut(strLogFileName);
    // remark: fall through on purpose
    switch (iLogLevel) {
      case 0 : textout->m_bShowWarnings = false;
      case 1 : textout->m_bShowMessages = false; break;
    }
    masterController.SetDebugOut(textout, true);
  }

  mainWindow.show();
  return app.exec();
}
