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

#include "../Tuvok/Basics/SystemInfo.h"
#include "../Tuvok/Basics/SysTools.h"
#include "../Tuvok/Renderer/GPUMemMan/GPUMemMan.h"
#include "../Tuvok/Scripting/Scripting.h"
#include "ImageVis3D.h"
#include <QtGui/QImageReader>

using namespace std;

void MainWindow::ShowVersions() {
  m_MasterController.DebugOut()->printf("Tuvok Version: %g %s %s",float(TUVOK_VERSION), TUVOK_VERSION_TYPE, TUVOK_DETAILS);
#ifdef TUVOK_SVN_VERSION
  m_MasterController.DebugOut()->printf("SVN Version: %i",int(TUVOK_SVN_VERSION));
#endif
  m_MasterController.DebugOut()->printf("ImageVis3D Version: %g %s",float(IV3D_VERSION), IV3D_VERSION_TYPE);
#ifdef IV3D_SVN_VERSION
  m_MasterController.DebugOut()->printf("SVN Version: %i",int(IV3D_SVN_VERSION));
#endif
  m_MasterController.DebugOut()->printf("QT Version: %s",QT_VERSION_STR);
}

void MainWindow::ShowGPUInfo(bool bWithExtensions) {
#if defined(_WIN32) && defined(USE_DIRECTX)
  if (DynamicDX::IsInitialized())
    m_MasterController.DebugOut()->printf("Direct3DX10 Version %i",DynamicDX::GetD3DX10Version());
  else
    m_MasterController.DebugOut()->printf("DirectX 10 not initialzed");
#endif

  if (RenderWindow::GetVendorString() == "") {
    m_MasterController.DebugOut()->printf("For the GL specs please open a GL renderwindow first!");
  } else {

    m_MasterController.DebugOut()->printf(RenderWindow::GetVendorString().c_str());
    m_MasterController.DebugOut()->printf("Maximum 3D texture size %i",int(RenderWindow::GetMax3DTexDims()));

    if (!bWithExtensions) return;

    if (RenderWindowGL::GetExtString() != "") {
      m_MasterController.DebugOut()->printf("Supported GL extensions:");

      vector< string > vExtensions = SysTools::Tokenize(RenderWindowGL::GetExtString());
      for (size_t i = 0;i<vExtensions.size();i++) {
        m_MasterController.DebugOut()->printf("  %s",vExtensions[i].c_str());
      }
      m_MasterController.DebugOut()->printf("%i extensions found",int(vExtensions.size()));
    }
  }
}


void MainWindow::ListSupportedImages() {
  m_MasterController.DebugOut()->printf("Supported image formats are:");
  QList<QByteArray> listImageFormats = QImageReader::supportedImageFormats();
  for (int i = 0;i<listImageFormats.size();i++) {
    QByteArray imageFormat = listImageFormats[i];
    QString qStrImageFormat(imageFormat);
    string strImageFormat = qStrImageFormat.toStdString();
    m_MasterController.DebugOut()->printf(strImageFormat.c_str());
  }
}

void MainWindow::ShowSysInfo() {
  m_MasterController.DebugOut()->printf("This is a %ubit build.", m_MasterController.MemMan()->GetBitWithMem());

  m_MasterController.DebugOut()->printf("CPU Memory: Total %llu MB, Usable %llu MB", m_MasterController.MemMan()->GetCPUMem()/(1024*1024), m_MasterController.SysInfo()->GetMaxUsableCPUMem()/(1024*1024));
  m_MasterController.DebugOut()->printf("    Used: %llu MB (%llu Bytes)",
                                          m_MasterController.MemMan()->GetAllocatedCPUMem()/(1024*1024),
                                          m_MasterController.MemMan()->GetAllocatedCPUMem());
  if (m_MasterController.MemMan()->GetAllocatedCPUMem() < m_MasterController.MemMan()->GetCPUMem() )
    m_MasterController.DebugOut()->printf("    Available: %llu MB", (m_MasterController.MemMan()->GetCPUMem()-m_MasterController.MemMan()->GetAllocatedCPUMem())/(1024*1024));

  m_MasterController.DebugOut()->printf("GPU Memory: Total %llu MB, Usable %llu MB", m_MasterController.MemMan()->GetGPUMem()/(1024*1024), m_MasterController.SysInfo()->GetMaxUsableGPUMem()/(1024*1024));
  m_MasterController.DebugOut()->printf("    Used: %llu MB (%llu Bytes)",
                                          m_MasterController.MemMan()->GetAllocatedGPUMem()/(1024*1024),
                                          m_MasterController.MemMan()->GetAllocatedGPUMem());
  if (m_MasterController.MemMan()->GetAllocatedGPUMem() < m_MasterController.MemMan()->GetGPUMem() )
    m_MasterController.DebugOut()->printf("    Available: %llu MB", (m_MasterController.MemMan()->GetGPUMem()-m_MasterController.MemMan()->GetAllocatedGPUMem())/(1024*1024));
}

void MainWindow::ClearDebugWin() {
  listWidget_DebugOut->clear();
}

void MainWindow::SetDebugViewMask() {
  m_MasterController.DebugOut()->SetOutput(checkBox_ShowDebugErrors->isChecked(),
                                           checkBox_ShowDebugWarnings->isChecked(),
                                           checkBox_ShowDebugMessages->isChecked(),
                                           checkBox_ShowDebugOther->isChecked());
}

void MainWindow::GetDebugViewMask() {

  bool bShowErrors;
  bool bShowWarnings;
  bool bShowMessages;
  bool bShowOther;

  m_MasterController.DebugOut()->GetOutput(bShowErrors,
                      bShowWarnings,
                      bShowMessages,
                      bShowOther);

  checkBox_ShowDebugErrors->setChecked(bShowErrors);
  checkBox_ShowDebugWarnings->setChecked(bShowWarnings);
  checkBox_ShowDebugMessages->setChecked(bShowMessages);
  checkBox_ShowDebugOther->setChecked(bShowOther);
}

void MainWindow::ParseAndExecuteDebugCommand() {
  bool bTemp = GetDebugOut()->ShowMessages();
  GetDebugOut()->SetShowMessages(true);

  m_MasterController.ScriptEngine()->ParseLine(lineEdit_DebugCommand->text().toStdString());
  
  GetDebugOut()->SetShowMessages(bTemp);
  if (listWidget_DebugOut->count() > 0) 
    listWidget_DebugOut->setCurrentRow(listWidget_DebugOut->count()-1);

  lineEdit_DebugCommand->clear();
}
