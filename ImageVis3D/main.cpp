//!    File   : main.cpp
//!    Author : Jens Krueger
//!             SCI Institute
//!             University of Utah
//!    Date   : September 2008
//
//!    Copyright (C) 2008 SCI Institute

#include "../Tuvok/StdTuvokDefines.h"

#include "../Tuvok/IO/Tuvok_QtPlugins.h"
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QString>
#include <QtWidgets/QApplication>
#include <QtGui/QClipboard>
#include <QtWidgets/QMessageBox>
#include <UI/ImageVis3D.h>
#include "../Tuvok/Controller/Controller.h"

#include "../Tuvok/Basics/SysTools.h"
#include "../Tuvok/DebugOut/TextfileOut.h"
#include "../Tuvok/DebugOut/ConsoleOut.h"

#if defined(_WIN32) && defined(USE_DIRECTX)
  #include "../Tuvok/Basics/DynamicDX.h"
#endif

/*
#ifdef _WIN32
  #if defined(DEBUG) || defined(_DEBUG)
    #define _CRTDBG_MAP_ALLOC
    #include <stdlib.h>
    #include <crtdbg.h>

    #ifndef DBG_NEW
      #define DBG_NEW new ( _NORMAL_BLOCK , __FILE__ , __LINE__ )
      #define new DBG_NEW
    #endif
  #endif
#endif
*/

// Include the following line for the Visual Leak detection tool
// #include <vld.h>

using namespace tuvok;


int main(int argc, char* argv[])
{
  #if defined(_WIN32) && defined(USE_DIRECTX)
    DynamicDX::InitializeDX();
  #endif

/*  
  // Enable run-time memory check for debug builds on windows
#ifdef _WIN32
  // CRT's memory leak detection on windows
  #if defined(DEBUG) || defined(_DEBUG)
  _CrtSetDbgFlag ( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF );
  _CrtSetReportMode( _CRT_ERROR, _CRTDBG_MODE_DEBUG );
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

  // if using a logfile inject that file-logger into the debug out chain
  if (bUseLogFile) {
    AbstrDebugOut *dbgOut;
    if(strLogFileName == "-") {
      dbgOut = new ConsoleOut();
    } else {
      dbgOut = new TextfileOut(SysTools::toWide(strLogFileName));
    }

    dbgOut->SetShowErrors(true);
    dbgOut->SetShowWarnings(iLogLevel > 0);
    dbgOut->SetShowMessages(iLogLevel > 1);

    {
      std::ostringstream loglevel;
      loglevel << "Loglevel: " << iLogLevel;
      dbgOut->printf(loglevel.str().c_str());
    }

    Controller::Instance().AddDebugOut(dbgOut);
  }
  MainWindow mainWindow(Controller::Instance(), strScriptFile!="", 0,
                        Qt::Window);

  if (strScriptFile != "") {
    bool bScriptResult =  mainWindow.RunLuaScript(SysTools::toWide(strScriptFile));
    if (!mainWindow.StayOpen()) {
      mainWindow.close();
      return (bScriptResult) ? 0 : 1;
    }
  }
#ifdef DETECTED_OS_LINUX
  // else: only do this check in interactive mode.
  else {
    if(!QFile::exists("./ImageVis3D")) {
      MESSAGE("Working directory wrong!  Applying hack.");
      // grab a copy of the full path w/ "ImageVis3D" lopped off the end.
      QDir d = QDir(argv[0]);
      std::string s;
      s.insert(0, d.absolutePath().toStdString());
      size_t iv3d = s.rfind("ImageVis3D");
      if(iv3d != std::string::npos) {
        std::string dir;
        dir.insert(0, s.c_str(), iv3d);
        MESSAGE("cd'ing to dir: %s", dir.c_str());
        if(!QDir::setCurrent(QString(dir.c_str()))) {
          WARNING("Could not set sane working directory!");
          const char* msg =
            "ImageVis3D's \"working directory\" is not the same as the "
            "directory it was started from, and I could not fix it.  "
            "If you've used a tarball "
            "installation of ImageVis3D, this is going to make it "
            "almost impossible for us to find our shaders, which means "
            "you won't be able to open a data set!\n\n"
            "This almost assuredly happened because you opened ImageVis3D "
            "by double-clicking it in KDE's file browser.  As outlined in "
            "KDE bug 131010, KDE does not properly set working directories.\n\n"
            "The only way we could presumably fix this is to provide "
            "distribution-specific binaries for all varieties of Linux out "
            "there.  Of course, given our limited resources this is "
            "impossible.  You can help us by commenting on the KDE bug about "
            "how unreasonable this behavior is and how difficult it is for a "
            "small development house to conform to this behavior.\n\n"
            "The URL is: https://bugs.kde.org/show_bug.cgi?id=131010.";
          QString err = QString(msg);
          QClipboard *c = QApplication::clipboard();
          if(c->text().isEmpty()) {
            c->setText("https://bugs.kde.org/show_bug.cgi?id=131010");
            err += QString("  Since your clipboard was empty anyway, I have "
                           "copied that URL into your clipboard's text.");
          }
          QMessageBox::warning(&mainWindow, "Invalid working directory", err);
        }
      }
    }
  }
#endif

  mainWindow.StartTimer();
  int iResult = app.exec();

  #if defined(_WIN32) && defined(USE_DIRECTX)
    DynamicDX::CleanupDX();
  #endif

  return iResult;
}

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