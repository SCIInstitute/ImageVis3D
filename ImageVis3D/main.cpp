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
//!    Date   : September 2008
//
//!    Copyright (C) 2008 SCI Institute

#include "../Tuvok/IO/Tuvok_QtPlugins.h"
#include <QtGui/QApplication>
#include <UI/ImageVis3D.h>
#include "../Tuvok/Controller/Controller.h"

#include "../Tuvok/Basics/SysTools.h"
#include "../Tuvok/DebugOut/TextfileOut.h"
#include "../Tuvok/DebugOut/ConsoleOut.h"
#include "../Tuvok/IO/IOManager.h"

#if defined(_WIN32) && defined(USE_DIRECTX)
  #include "../Tuvok/Basics/DynamicDX.h"
#endif

/*
#ifdef _WIN32
  // CRT's memory leak detection on windows
  #if defined(DEBUG) || defined(_DEBUG)
    #include <crtdbg.h>
  #endif
#endif
*/


int main(int argc, char* argv[])
{
  #if defined(_WIN32) && defined(USE_DIRECTX)
    DynamicDX::InitializeDX();
  #endif

  /*
  // Enable run-time memory check for debug builds on windows
  #ifdef _WIN32
    #if defined(DEBUG) | defined(_DEBUG)
      _CrtSetDbgFlag( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF );
    #endif
  #endif
  */

  // get command line paramers 
  SysTools::CmdLineParams parameters(argc, argv);

  // start a logfile debug out if requested
  std::string strLogFileName;
  bool bUseLogFile = parameters.GetValue("LOG",strLogFileName);
  int iLogLevel = 0;
  std::string strScriptFile = "";
  parameters.GetValue("LOGLEVEL",iLogLevel);
  parameters.GetValue("SCRIPT",strScriptFile);

  // create the QT window
  QApplication app( argc, argv );
  MainWindow mainWindow(Controller::Instance(), strScriptFile!="", 0,
                        Qt::Window);
  
  // if using a logfile inject that file-logger into the debug out chain
  if (bUseLogFile) {
    AbstrDebugOut *dbgOut;
    if(strLogFileName == "-") {
      dbgOut = new ConsoleOut();
    } else {
      dbgOut = new TextfileOut(strLogFileName);
    }
  
    dbgOut->SetShowErrors(true);
    dbgOut->SetShowWarnings(iLogLevel > 0);
    dbgOut->SetShowMessages(iLogLevel > 1);

    dbgOut->printf("Loglevel:%i\n",iLogLevel);

    Controller::Instance().AddDebugOut(dbgOut);
  }

  // open the QT window
  mainWindow.show();

  if (strScriptFile != "") {
    bool bScriptResult =  mainWindow.RunScript(strScriptFile);
    if (!mainWindow.StayOpen()) {
      mainWindow.close();
      return (bScriptResult) ? 0 : 1;
    }
  }

  int iResult = app.exec();

  #if defined(_WIN32) && defined(USE_DIRECTX)
    DynamicDX::CleanupDX();
  #endif

  return iResult;
}
