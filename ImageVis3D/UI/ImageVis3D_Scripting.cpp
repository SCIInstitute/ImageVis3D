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

#include <algorithm>
#include <cctype>
#include <iterator>
#include "ImageVis3D.h"
#include "../Tuvok/Basics/SysTools.h"
#include "../Tuvok/Basics/Appendix.h"
#include "../Tuvok/Scripting/Scripting.h"
#ifndef DETECTED_OS_WINDOWS
#pragma GCC visibility push(default)
#endif
#include <QtOpenGL/QtOpenGL>
#ifndef DETECTED_OS_WINDOWS
#pragma GCC visibility pop
#endif

using namespace std;

void MainWindow::RegisterCalls(Scripting* pScriptEngine) {
  pScriptEngine->RegisterCommand(this, "clear", "", "clear this window");
  pScriptEngine->RegisterCommand(this, "versions", "", "print version information");
  pScriptEngine->RegisterCommand(this, "gpuinfo", "","print basic information about the GPU, the driver and the rendering APIs");
  pScriptEngine->RegisterCommand(this, "gpuinfoext", "","print extensive information about the GPU, the driver and the rendering APIs");
  pScriptEngine->RegisterCommand(this, "sysinfo", "","print information about the system and the mem usage");
  pScriptEngine->RegisterCommand(this, "geoformats", "","print a list of the supported geometry formats");
  pScriptEngine->RegisterCommand(this, "volumeformats", "","print a list of the supported volume formats");  
  pScriptEngine->RegisterCommand(this, "imageformats", "","print a list of the supported image formats");
  pScriptEngine->RegisterCommand(this, "open", "sourcefile [targetfile]","open the sourcefile and write it into targetfile if conversion is required");
  pScriptEngine->RegisterCommand(this, "open1d", "sourcefile","open 1D transfer function sourcefile");
  pScriptEngine->RegisterCommand(this, "open2d", "sourcefile","open 2D transfer function sourcefile");
  pScriptEngine->RegisterCommand(this, "setiso", "isovalue","set an isovalue from 0 to 1");
  pScriptEngine->RegisterCommand(this, "setcviso", "isovalue",
                                       "set an isovalue from 0 to 1");
  pScriptEngine->RegisterCommand(this, "mode1d", "","switch to 1D transfer function rendering");
  pScriptEngine->RegisterCommand(this, "mode2d", "","switch to 2D transfer function rendering");
  pScriptEngine->RegisterCommand(this, "modeiso", "","switch to isomode rendering");
  pScriptEngine->RegisterCommand(this, "export", "targetfile [LOD]","export the current dataset into 'targetfile' using LOD level 'LOD' default is 0 (max quality) the filetype is determined by the extension");
  pScriptEngine->RegisterCommand(this, "exportiso", "targetfile [LOD]","export the current isosurface into 'targetfile' using LOD level 'LOD' default is 0 (max quality)");
  pScriptEngine->RegisterCommand(this, "compare", "file1 file2","compare file1 and file2");
  pScriptEngine->RegisterCommand(this, "close", "","close the current datawindow");
  pScriptEngine->RegisterCommand(this, "resize", "sizeX sizeY","resize the current data window");
  pScriptEngine->RegisterCommand(this, "rotateX", "angle","rotate the data by \"angle\" degree around the x axis");
  pScriptEngine->RegisterCommand(this, "rotateY", "angle","rotate the data by \"angle\" degree around the y axis");
  pScriptEngine->RegisterCommand(this, "rotateZ", "angle","rotate the data by \"angle\" degree around the z axis");
  pScriptEngine->RegisterCommand(this, "translate", "x y z","translate the data by [x,y,z]");
  pScriptEngine->RegisterCommand(this, "reset", "","reset all rendering parameters to their inital state");
  pScriptEngine->RegisterCommand(this, "setLOD", "true/false", "enabled/disables the LOD meachnism and always renders only the high res dataset");
  pScriptEngine->RegisterCommand(this, "setStereo", "true/false", "enabled/disables stereo rendering");
  pScriptEngine->RegisterCommand(this, "setStereoMode", "0/1/2/3", "choose stereo mode, see radiobox in stereo widget for modes");
  pScriptEngine->RegisterCommand(this, "setStereoDisparity", "disparity", "set the eye distance in stereo mode");
  pScriptEngine->RegisterCommand(this, "incStereoFLength", "deltafocalLength", "add a delta to the focal length in stereo mode");
  pScriptEngine->RegisterCommand(this, "incStereoDisparity", "deltadisparity", "add a delta to the eye distance in stereo mode");
  pScriptEngine->RegisterCommand(this, "setStereoFLength", "focalLength", "set the focal length in stereo mode");
  pScriptEngine->RegisterCommand(this, "initAFStereo", "", "active left eye in alternating frame stereo mode");
  pScriptEngine->RegisterCommand(this, "toggleAFStereo", "", "toggle active eye in alternating frame stereo mode");
  pScriptEngine->RegisterCommand(this, "capturesingle", "targetfile", "capture a single image into targetfile");
  pScriptEngine->RegisterCommand(this, "capturesequence", "targetfile", "capture a single image into targetfile_counter");
  pScriptEngine->RegisterCommand(this, "stayopen", "", "do not close the application at the end of the script");
  pScriptEngine->RegisterCommand(this, "pack", "source ... target", "pack the files source0 to sourceN into a single file target");
  pScriptEngine->RegisterCommand(this, "upload", "source [target]", "upload the file 'source' to the debug server with the name 'target' (by default a unique filename is generated automatically)");
  pScriptEngine->RegisterCommand(this, "delete", "file", "delete the file 'file'");
  pScriptEngine->RegisterCommand(this, "quit", "", "quit ImageVis3D");
  pScriptEngine->RegisterCommand(this, "lighting", "[true/false]", "Enable, disable, or toggle (no args) lighting");
  pScriptEngine->RegisterCommand(this, "samplingrate", "percent", "Sets the sampling rate; default is 100");
}

bool MainWindow::Execute(const std::string& strCommand, const std::vector< std::string >& strParams, std::string& strMessage) {
  bool bResult = true;
  QCoreApplication::processEvents();
  strMessage = "";
  if (strCommand == "clear")           { ClearDebugWin(); } else
  if (strCommand == "versions")        { ShowVersions(); } else
  if (strCommand == "gpuinfo")         { ShowGPUInfo(false); } else
  if (strCommand == "gpuinfoext")      { ShowGPUInfo(true); } else
  if (strCommand == "sysinfo")         { ShowSysInfo();} else
  if (strCommand == "imageformats")    { ListSupportedImages();} else
  if (strCommand == "volumeformats")   { ListSupportedVolumes();} else    
  if (strCommand == "geoformats")      { ListSupportedGeometry();} else    
  if (strCommand == "open")            { bResult = LoadDataset(strParams); if (!bResult) {strMessage = "Unable to load dataset file "+strParams[0];}} else
  if (strCommand == "open1d")          { bResult = Transfer1DLoad(strParams[0]); if (!bResult) {strMessage = "Unable to load transfer function "+strParams[0];}} else
  if (strCommand == "open2d")          { bResult = Transfer2DLoad(strParams[0]); if (!bResult) {strMessage = "Unable to load transfer function "+strParams[0];}} else
  if (strCommand == "setiso")          { SetIsoValue(SysTools::FromString<float>(strParams[0]));} else
  if (strCommand == "setcviso")        { SetClearViewIsoValue(SysTools::FromString<float>(strParams[0])); } else
  if (strCommand == "mode1d")          { Use1DTrans();} else
  if (strCommand == "mode2d")          { Use2DTrans();} else
  if (strCommand == "modeiso")         { UseIso();} else
  if (strCommand == "export")          { ExportDataset( (strParams.size()>1) ? atoi(strParams[1].c_str()) : 0, strParams[0]); } else
  if (strCommand == "exportiso")       { ExportIsosurface( (strParams.size()>1) ? atoi(strParams[1].c_str()) : 0, strParams[0]); } else
  if (strCommand == "compare")         { CompareFiles(strParams[0], strParams[1]);} else
  if (strCommand == "close")           { CloseCurrentView();} else
  if (strCommand == "resize")          { ResizeCurrentView(atoi(strParams[0].c_str()), atoi(strParams[1].c_str()));} else
  if (strCommand == "rotateX")         { RotateCurrentViewX(atof(strParams[0].c_str()));} else
  if (strCommand == "rotateY")         { RotateCurrentViewY(atof(strParams[0].c_str()));} else
  if (strCommand == "rotateZ")         { RotateCurrentViewZ(atof(strParams[0].c_str()));} else
  if (strCommand == "translate")       { TranslateCurrentView(atof(strParams[0].c_str()), atof(strParams[1].c_str()), atof(strParams[2].c_str()));} else
  if (strCommand == "reset")           { ResetRenderingParameters();} else
  if (strCommand == "capturesingle")   { bResult = CaptureFrame(strParams[0]); if (!bResult) {strMessage = "Unable to save file "+strParams[0];}} else
  if (strCommand == "capturesequence") { bResult = CaptureSequence(strParams[0]); if (!bResult) {strMessage = "Unable to save file "+strParams[0];}} else
  if (strCommand == "stayopen")        { m_bStayOpenAfterScriptEnd = true;} else
  if (strCommand == "pack")            { bResult = Pack(strParams);} else
  if (strCommand == "upload")          { bResult = FtpTransfer(strParams[0], (strParams.size()>1) ? strParams[1].c_str() : GenUniqueName("Script", "data"), false );} else
  if (strCommand == "delete")          { bResult = remove(strParams[0].c_str()) == 0;} else
  if (strCommand == "setLOD")            { if (m_pActiveRenderWin) m_pActiveRenderWin->GetRenderer()->SetCaptureMode(SysTools::ToLowerCase(strParams[0]) != "true"); } else
  if (strCommand == "setStereo")         { checkBox_Stereo->setChecked(SysTools::ToLowerCase(strParams[0]) == "true"); ToggleStereoRendering(); } else
  if (strCommand == "setStereoMode")     { SetStereoMode(SysTools::FromString<unsigned int>(strParams[0])); } else  
  if (strCommand == "setStereoFLength"){ SetStereoFocalLength(SysTools::FromString<float>(strParams[0])); } else
  if (strCommand == "setStereoDisparity"){ SetStereoEyeDistance(SysTools::FromString<float>(strParams[0])); } else
  if (strCommand == "incStereoFLength")     { IncStereoFocalLength(SysTools::FromString<float>(strParams[0])); } else
  if (strCommand == "incStereoDisparity")   { IncStereoEyeDistance(SysTools::FromString<float>(strParams[0])); } else
  if (strCommand == "initAFStereo")    { InitAFStereo(); } else
  if (strCommand == "toggleAFStereo")  { ToggleAFStereo(); } else
  if (strCommand == "quit")            { bResult = close();} else
  if (strCommand == "samplingrate")    { SetSampleRate(SysTools::FromString<unsigned int>(strParams[0])); } else
  if (strCommand == "lighting")        {
    if(!strParams.empty()) {
      std::string arg;
      std::transform(strParams[0].begin(), strParams[0].end(),
                     std::back_inserter(arg), (int(*)(int))std::tolower);
      if(arg == "on" || arg == "yes" || arg == "true" || arg == "1") {
        SetLighting(true);
      } else {
        SetLighting(false);
      }
    } else {
      ToggleLighting();
    }
  } else {
    return false;
  }

  QCoreApplication::processEvents();

  return true;
}

void MainWindow::SetStereoMode(unsigned int iMode) {
  if (m_pActiveRenderWin == NULL || iMode >= (unsigned int)(AbstrRenderer::SM_INVALID) ) return;
  m_pActiveRenderWin->GetRenderer()->SetStereoMode(AbstrRenderer::EStereoMode(iMode));
}

void MainWindow::SetStereoEyeDistance(float fEyeDist) {
  if (m_pActiveRenderWin == NULL) return;
  m_pActiveRenderWin->GetRenderer()->SetStereoEyeDist(fEyeDist);
}

void MainWindow::SetStereoFocalLength(float fLength) {
  if (m_pActiveRenderWin == NULL) return;
  m_pActiveRenderWin->GetRenderer()->SetStereoFocalLength(fLength);
}

void MainWindow::IncStereoEyeDistance(float fEyeDistDelta) {
  if (m_pActiveRenderWin == NULL) return;
  m_pActiveRenderWin->GetRenderer()->SetStereoEyeDist(m_pActiveRenderWin->GetRenderer()->GetStereoEyeDist() + fEyeDistDelta);
}

void MainWindow::IncStereoFocalLength(float fLengthDelta) {
  if (m_pActiveRenderWin == NULL) return;
  m_pActiveRenderWin->GetRenderer()->SetStereoFocalLength(m_pActiveRenderWin->GetRenderer()->GetStereoFocalLength() + fLengthDelta);
}

bool MainWindow::Pack(const std::vector< std::string >& strParams) {
  vector<string> vFiles(strParams.begin(), strParams.end()-1);
  Appendix a(strParams[strParams.size()-1], vFiles);
  return a.IsOK();
}

bool MainWindow::RunScript(const std::string& strFilename) {
  return m_MasterController.ScriptEngine()->ParseFile(strFilename);
}


void MainWindow::InitAFStereo() {
  if (m_pActiveRenderWin == NULL) return;
  m_pActiveRenderWin->GetRenderer()->InitStereoFrame();
}

void MainWindow::ToggleAFStereo() {
  if (m_pActiveRenderWin == NULL) return;
  m_pActiveRenderWin->GetRenderer()->ToggleStereoFrame();
}
