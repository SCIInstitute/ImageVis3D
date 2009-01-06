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


//!    File   : ImageVis3D_Scripting.cpp
//!    Author : Jens Krueger
//!             SCI Institute
//!             University of Utah
//!    Date   : September 2008
//
//!    Copyright (C) 2008 SCI Institute

#include "ImageVis3D.h"
#include "../Tuvok/Basics/SysTools.h"
#include <QtOpenGL/QtOpenGL>

using namespace std;

bool MainWindow::RegisterCalls(Scripting* pScriptEngine) {
  pScriptEngine->RegisterCommand(this, "clear", "", "clear this window");
  pScriptEngine->RegisterCommand(this, "versions", "", "print version information");
  pScriptEngine->RegisterCommand(this, "glinfo", "","print information about the supported OpenGL extension");
  pScriptEngine->RegisterCommand(this, "sysinfo", "","print information about the system and the mem usage");
  pScriptEngine->RegisterCommand(this, "sysinfo", "","print information about the system and the mem usage");
  pScriptEngine->RegisterCommand(this, "open", "sourcefile [targetfile]","open the sourcefile and write it into targetfile if conversion is required");
  pScriptEngine->RegisterCommand(this, "open1d", "sourcefile","open 1D transfer function sourcefile");
  pScriptEngine->RegisterCommand(this, "open2d", "sourcefile","open 2D transfer function sourcefile");
  pScriptEngine->RegisterCommand(this, "setiso", "isovalue","set an isovalue from 0 to 1");
  pScriptEngine->RegisterCommand(this, "mode1d", "","switch to 1D transfer function rendering");
  pScriptEngine->RegisterCommand(this, "mode2d", "","switch to 2D transfer function rendering");
  pScriptEngine->RegisterCommand(this, "modeiso", "","switch to isomode rendering");
  pScriptEngine->RegisterCommand(this, "close", "","close the current datawindow");
  pScriptEngine->RegisterCommand(this, "resize", "sizeX sizeY","resize the current data window");
  pScriptEngine->RegisterCommand(this, "rotateX", "angle","rotate the data by \"angle\" degree around the x axis");
  pScriptEngine->RegisterCommand(this, "rotateY", "angle","rotate the data by \"angle\" degree around the x axis");
  pScriptEngine->RegisterCommand(this, "rotateZ", "angle","rotate the data by \"angle\" degree around the x axis");
  pScriptEngine->RegisterCommand(this, "translate", "x y z","translate the data by [x,y,z]");
  pScriptEngine->RegisterCommand(this, "capturesingle", "targetfile", "capture a single image into targetfile");
  pScriptEngine->RegisterCommand(this, "capturesequence", "targetfile", "capture a single image into targetfile_counter");
  pScriptEngine->RegisterCommand(this, "stayopen", "", "do not close the app after the end of the script");
  pScriptEngine->RegisterCommand(this, "quit", "", "quit ImageVis3D");
  return true;
}

bool MainWindow::Execute(const std::string& strCommand, const std::vector< std::string >& strParams) {
  if (strCommand == "clear")           { ClearDebugWin(); } else
  if (strCommand == "versions")        { ShowVersions(); } else
  if (strCommand == "glinfo")          { ShowGLInfo(); } else
  if (strCommand == "sysinfo")         { ShowSysInfo();} else
  if (strCommand == "open")            { return LoadDataset(strParams);} else
  if (strCommand == "open1d")          { return Transfer1DLoad(strParams[0]);} else
  if (strCommand == "open2d")          { return Transfer2DLoad(strParams[0]);} else
  if (strCommand == "setiso")          { SetIsoValue(float(atof(strParams[0].c_str())));} else
  if (strCommand == "mode1d")          { Use1DTrans();} else
  if (strCommand == "mode2d")          { Use2DTrans();} else
  if (strCommand == "modeiso")         { UseIso();} else
  if (strCommand == "close")           { CloseCurrentView();} else
  if (strCommand == "resize")          { ResizeCurrentView(atoi(strParams[0].c_str()), atof(strParams[1].c_str()));} else
  if (strCommand == "rotateX")         { RotateCurrentViewX(atof(strParams[0].c_str()));} else
  if (strCommand == "rotateY")         { RotateCurrentViewY(atof(strParams[0].c_str()));} else
  if (strCommand == "rotateZ")         { RotateCurrentViewZ(atof(strParams[0].c_str()));} else
  if (strCommand == "translate")       { TranslateCurrentView(atof(strParams[0].c_str()), atof(strParams[1].c_str()), atof(strParams[2].c_str()));} else
  if (strCommand == "capturesingle")   { return CaptureFrame(strParams[0]);} else
  if (strCommand == "capturesequence") { return CaptureSequence(strParams[0]);} else
  if (strCommand == "stayopen")        { m_bStayOpenAfterScriptEnd = true;} else
  if (strCommand == "quit")            { return close();} else
    return false;
  return true;
}

bool MainWindow::RunScript(const std::string& strFilename) {
  return m_MasterController.ScriptEngine()->ParseFile(strFilename);
}
