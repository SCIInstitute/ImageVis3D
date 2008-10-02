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
  string strParameter;

  if (strCommand.find_first_of(" ") != string::npos) {
    strParameter = strCommand.substr(strCommand.find_first_of(" ")+1,strCommand.length()-(strCommand.find_first_of(" ")+1));
    strCommand   = strCommand.substr(0,strCommand.find_first_of(" "));
  }

  if (ParseCommand(strCommand,strParameter)) 
    m_DebugOut->printf("OK");
  else
    m_DebugOut->printf("Input \"%s\" not understood, try \"help\"!", lineEdit_DebugCommand->text().toStdString().c_str());

  m_DebugOut->m_bShowMessages = bTemp;
  lineEdit_DebugCommand->clear();
}

bool MainWindow::ParseCommand(string strCommand, string strParam) {


  strCommand = SysTools::ToLowerCase(strCommand);
  strParam   = SysTools::ToLowerCase(strParam);

  if (strCommand == "help") {
    m_DebugOut->printf("Command Listing:");
    m_DebugOut->printf("\"help\"                  : this help screen");
    m_DebugOut->printf("\"clear\"                 : clear this window");
    m_DebugOut->printf("\"versions\"              : print version information");
    m_DebugOut->printf("\"glinfo\"                : print information about the supported OpenGL extension");
    m_DebugOut->printf("\"sysinfo\"               : print information about the system and the mem usage");
    m_DebugOut->printf("\"seterrorlog\" on/off    : toggle recording of errors");
    m_DebugOut->printf("\"setwarninglog\" on/off  : toggle recording of warnings");
    m_DebugOut->printf("\"setemessagelog\" on/off : toggle recording of messages");
    m_DebugOut->printf("\"printerrorlog\"         : print recorded errors");
    m_DebugOut->printf("\"printwarninglog\"       : print recorded warnings");
    m_DebugOut->printf("\"printmessagelog\"       : print recorded messages");
    m_DebugOut->printf("\"clearerrorlog\"         : clear recorded errors");
    m_DebugOut->printf("\"clearwarninglog\"       : clear recorded warnings");
    m_DebugOut->printf("\"clearmessagelog\"       : clear recorded messages");
    return true;
  }
  if (strCommand == "clear") {
    ClearDebugWin();
    return true;
  }
  if (strCommand == "versions") {      
    m_DebugOut->printf("ImageVis3D Version: %s",IV3D_VERSION);
    m_DebugOut->printf("QT Version: %s",QT_VERSION_STR);    
    return true;
  }
  if (strCommand == "seterrorlog") {
    if (strParam != "on" && strParam != "off") return false;
    m_DebugOut->SetListRecordingErrors(strParam == "on");
    if (m_DebugOut->GetListRecordingErrors()) m_DebugOut->printf("current state: true"); else m_DebugOut->printf("current state: false");
    return true;
  }
  if (strCommand == "setwarninglog") {
    if (strParam != "on" && strParam != "off") return false;
    m_DebugOut->SetListRecordingWarnings(strParam == "on");
    if (m_DebugOut->GetListRecordingWarnings()) m_DebugOut->printf("current state: true"); else m_DebugOut->printf("current state: false");
    return true;
  }
  if (strCommand == "setmessagelog") {
    if (strParam != "on" && strParam != "off") return false;
    m_DebugOut->SetListRecordingMessages(strParam == "on");
    if (m_DebugOut->GetListRecordingMessages()) m_DebugOut->printf("current state: true"); else m_DebugOut->printf("current state: false");
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
  if (strCommand == "glinfo") {
    if (m_ActiveRenderWin == NULL) {
      m_DebugOut->printf("please open a renderwindow first");
    } else {

      m_DebugOut->printf("Printing GL extensions:");

      const char *extensions = (const char*)glGetString(GL_EXTENSIONS);

      unsigned int iExtCount = 0;
      string strExtension = "";
      while (extensions[0]) {
        if (extensions[0] == ' ' || extensions[0] == '\0') {
          m_DebugOut->printf("  %s",strExtension.c_str());
          strExtension = "";
          iExtCount++;
        } else strExtension += extensions[0];
        extensions++;
      }
      m_DebugOut->printf("%ui extensions found",iExtCount);
    }
    return true;
  }
  if (strCommand == "sysinfo") {
    m_DebugOut->printf("This a %ibit build.", m_MasterController.MemMan()->GetBitWithMem());

    m_DebugOut->printf("CPU Memory: Total %llu MB, Usable %llu MB", m_MasterController.MemMan()->GetCPUMem()/(1024*1024), m_MasterController.SysInfo()->GetMaxUsableCPUMem()/(1024*1024));
    m_DebugOut->printf("    Used: %llu MB (%llu Bytes)", 
      m_MasterController.MemMan()->GetAllocatedCPUMem()/(1024*1024),
      m_MasterController.MemMan()->GetAllocatedCPUMem());
    if (m_MasterController.MemMan()->GetAllocatedCPUMem() < m_MasterController.MemMan()->GetCPUMem() )
      m_DebugOut->printf("    Available: %llu MB", (m_MasterController.MemMan()->GetCPUMem()-m_MasterController.MemMan()->GetAllocatedCPUMem())/(1024*1024));

    m_DebugOut->printf("GPU Memory: Total %llu MB, Usable %llu MB", m_MasterController.MemMan()->GetGPUMem()/(1024*1024), m_MasterController.SysInfo()->GetMaxUsableGPUMem()/(1024*1024));
    m_DebugOut->printf("    Used: %llu MB (%llu Bytes)", 
      m_MasterController.MemMan()->GetAllocatedGPUMem()/(1024*1024),
      m_MasterController.MemMan()->GetAllocatedGPUMem());
    if (m_MasterController.MemMan()->GetAllocatedGPUMem() < m_MasterController.MemMan()->GetGPUMem() ) 
      m_DebugOut->printf("    Available: %llu MB", (m_MasterController.MemMan()->GetGPUMem()-m_MasterController.MemMan()->GetAllocatedGPUMem())/(1024*1024));

    return true;
  }
  return false;
}