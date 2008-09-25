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


//!    File   : ImageVis3D_DebugWindow.cpp
//!    Author : Jens Krueger
//!             SCI Institute
//!             University of Utah
//!    Date   : September 2008
//
//!    Copyright (C) 2008 SCI Institute

#include "ImageVis3D.h"
#include <Basics/SysTools.h>
#include <QtOpenGL/QtOpenGL>

using namespace std;

void MainWindow::ClearDebugWin() {
  listWidget_3->clear();
}


void MainWindow::SetDebugViewMask() {
  m_DebugOut->m_bShowErrors = checkBox_ShowDebugErrors->isChecked();
  m_DebugOut->m_bShowWarnings = checkBox_ShowDebugWarnings->isChecked();
  m_DebugOut->m_bShowMessages = checkBox_ShowDebugMessages->isChecked();
  m_DebugOut->m_bShowOther = checkBox_ShowDebugOther->isChecked();
}


void MainWindow::GetDebugViewMask() {
  checkBox_ShowDebugErrors->setChecked(m_DebugOut->m_bShowErrors);
  checkBox_ShowDebugWarnings->setChecked(m_DebugOut->m_bShowWarnings);
  checkBox_ShowDebugMessages->setChecked(m_DebugOut->m_bShowMessages);
  checkBox_ShowDebugOther->setChecked(m_DebugOut->m_bShowOther);
}


void MainWindow::ParseAndExecuteDebugCommand() {
  bool bTemp = m_DebugOut->m_bShowMessages;
  m_DebugOut->m_bShowMessages = true;

  if (m_DebugOut != m_MasterController.DebugOut())  {
    m_DebugOut->printf("Debug out is currently redirected to another debug out.");
  }

  // TODO
  string strCommand = lineEdit_DebugCommand->text().toStdString();

  if (ParseCommand(strCommand)) 
    m_DebugOut->printf("OK");
  else
    m_DebugOut->printf("Input \"%s\" not understood, try \"help\"!", strCommand.c_str());

  m_DebugOut->m_bShowMessages = bTemp;
  lineEdit_DebugCommand->clear();
}

bool MainWindow::ParseCommand(string strCommand) {
  strCommand = SysTools::ToLowerCase(strCommand);

  if (strCommand == "help") {
    m_DebugOut->printf("Command Listing:");
    m_DebugOut->printf("\"help\"             : this help screen");
    m_DebugOut->printf("\"clear\"            : clear this window");
    m_DebugOut->printf("\"getglinfo\"        : print information about the supported OpenGL extension");
    m_DebugOut->printf("\"toggleerrorlog\"   : toggle recording of errors");
    m_DebugOut->printf("\"togglewarninglog\" : toggle recording of warnings");
    m_DebugOut->printf("\"togglemessagelog\" : toggle recording of messages");
    m_DebugOut->printf("\"geterrorlog\"      : get recording of error status");
    m_DebugOut->printf("\"getwarninglog\"    : get recording of warnings status");
    m_DebugOut->printf("\"getmessagelog\"    : get recording of messages status");
    m_DebugOut->printf("\"printerrorlog\"    : print recorded errors");
    m_DebugOut->printf("\"printwarninglog\"  : print recorded warnings");
    m_DebugOut->printf("\"printmessagelog\"  : print recorded messages");
    m_DebugOut->printf("\"clearerrorlog\"    : clear recorded errors");
    m_DebugOut->printf("\"clearwarninglog\"  : clear recorded warnings");
    m_DebugOut->printf("\"clearmessagelog\"  : clear recorded messages");
    return true;
  }
  if (strCommand == "clear") {
    ClearDebugWin();
    return true;
  }
  if (strCommand == "toggleerrorlog") {
    m_DebugOut->SetListRecordingErrors(!m_DebugOut->GetListRecordingErrors());
    return true;
  }
  if (strCommand == "togglewarninglog") {
    m_DebugOut->SetListRecordingWarnings(!m_DebugOut->GetListRecordingWarnings());
    return true;
  }
  if (strCommand == "togglemessagelog") {
    m_DebugOut->SetListRecordingMessages(!m_DebugOut->GetListRecordingMessages());
    return true;
  }
  if (strCommand == "geterrorlog") {
    if (m_DebugOut->GetListRecordingErrors()) 
      m_DebugOut->printf("true");
    else 
      m_DebugOut->printf("false");
    return true;
  }
  if (strCommand == "getwarninglog") {
    if (m_DebugOut->GetListRecordingWarnings()) 
      m_DebugOut->printf("true");
    else 
      m_DebugOut->printf("false");
    return true;
  }
  if (strCommand == "getmessagelog") {
    if (m_DebugOut->GetListRecordingMessages()) 
      m_DebugOut->printf("true");
    else 
      m_DebugOut->printf("false");
    return true;
  }
  if (strCommand == "printerrorlog") {
    m_DebugOut->PrintErrorList();
    return true;
  }
  if (strCommand == "printwarninglog") {
    m_DebugOut->PrintWarningList();
    return true;
  }
  if (strCommand == "printmessagelog") {
    m_DebugOut->PrintMessageList();
    return true;
  }
  if (strCommand == "clearerrorlog") {
    m_DebugOut->ClearErrorList();
    return true;
  }
  if (strCommand == "clearwarninglog") {
    m_DebugOut->ClearWarningList();
    return true;
  }
  if (strCommand == "clearmessagelog") {
    m_DebugOut->ClearMessageList();
    return true;
  }
  if (strCommand == "getglinfo") {
    if (m_ActiveRenderWin == NULL) {
      m_DebugOut->printf("please open a renderwindow first");
    } else {

      const char *extensions = (const char*)glGetString(GL_EXTENSIONS);

      string strExtension = "";
      while (extensions[0]) {
        if (extensions[0] == ' ' || extensions[0] == '\0') {
          m_DebugOut->printf(strExtension.c_str());
          strExtension = "";
        } else strExtension += extensions[0];
        extensions++;
      }
    }
    return true;
  }
  return false;
}