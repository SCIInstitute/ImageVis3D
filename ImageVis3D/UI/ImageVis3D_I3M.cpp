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


//!    File   : ImageVis3D_I3M.cpp
//!    Author : Jens Krueger
//!             DFKI & MMCI, Saarbruecken
//!             SCI Institute, University of Utah
//!    Date   : July 2009
//
//!    Copyright (C) 2009 DFKI, MMCI, SCI Institute

#include "ImageVis3D.h"

#include "../Tuvok/Basics/SysTools.h"
#include "../Tuvok/Controller/Controller.h"
#include "../Tuvok/IO/IOManager.h"
#include "../Tuvok/IO/uvfDataset.h"


#include <QtGui/QFileDialog>
#include <QtCore/QSettings>
#include <QtGui/QMessageBox>

#include "PleaseWait.h"

using namespace tuvok;
using namespace std;

string MainWindow::ConvertTF(const string& strSource1DTFilename,
                             const string& strTargetDir, 
                             const UVFDataset* currentDataset,
                             PleaseWaitDialog& pleaseWait) {

  pleaseWait.SetText("Converting transfer function, please wait  ...");

  string filenameOnly = SysTools::ChangeExt(
      SysTools::GetFilename(currentDataset->Filename()),"i3m.1dt"
      );

  string strTarget1DTFilename = strTargetDir+"/"+filenameOnly;
  MESSAGE("Converting transferfunction to %s",strTarget1DTFilename.c_str());

  // resample 1D tf to 8bit
  TransferFunction1D tfIn(strSource1DTFilename);
  tfIn.Resample(256);
  if (!tfIn.Save(strTarget1DTFilename)) return "";


  MESSAGE("Saved 8bit transferfunction to %s",strTarget1DTFilename.c_str());
  return strTarget1DTFilename;
}

string MainWindow::ConvertDataToI3M(const UVFDataset* currentDataset,
                                    const string& strTargetDir,
                                    PleaseWaitDialog& pleaseWait,
                                    bool bOverrideExisting) {


  pleaseWait.SetText("Converting:"+
                     QString(currentDataset->Filename().c_str()));

  // UVF to I3M

  // first, find the smalest LOD with every dimension
  // larger or equal to 128 (if possible)
  int iLODLevel = int(currentDataset->GetLODLevelCount())-1;
  for (;iLODLevel>0;iLODLevel--) {
    UINTVECTOR3 vLODSize = UINTVECTOR3(
                                currentDataset->GetDomainSize(iLODLevel)
                           );
    if (vLODSize.x >= 128 &&
        vLODSize.y >= 128 &&
        vLODSize.z >= 128) break;
  }

  string filenameOnly = SysTools::GetFilename(currentDataset->Filename());
  string strTargetFilename = strTargetDir+"/"+
                             SysTools::ChangeExt(filenameOnly,"i3m");

  if (!bOverrideExisting && SysTools::FileExists(strTargetFilename)) {
    strTargetFilename = SysTools::FindNextSequenceName(
                                                    strTargetFilename
                                                    );
  }

  if (!Controller::Instance().IOMan()->ExportDataset(currentDataset, 
                                                     iLODLevel, 
                                                     strTargetFilename, 
                                                     m_strTempDir)) {
    return "";
  }

  return strTargetFilename;
}


void MainWindow::TransferToI3M() {
  if (!m_pActiveRenderWin) return;

  const UVFDataset* currentDataset = dynamic_cast<UVFDataset*>(&(m_pActiveRenderWin->GetRenderer()->GetDataset()));

  if (currentDataset) {
    QSettings settings;
    QString strLastDir = settings.value("Folders/I3DMServer",
                                        ".").toString();

    QString directoryName =
      QFileDialog::getExistingDirectory(this, "Select Dataset Server folder.",
                                        strLastDir);

    if (directoryName.isEmpty()) return;

    string strTargetDir = string(directoryName.toAscii().data());
    
    settings.setValue("Folders/I3MServer", directoryName);


    PleaseWaitDialog pleaseWait(this);
    pleaseWait.SetText("Preparing data  ...");
    pleaseWait.AttachLabel(&m_MasterController);

    string targetFile = ConvertDataToI3M(currentDataset,strTargetDir,
                                         pleaseWait, true);

    if (targetFile == "") {
      QMessageBox errorMessage;
      errorMessage.setText("Unable to convert the dataset "
                           "into the given directory.");
      errorMessage.setIcon(QMessageBox::Critical);
      errorMessage.exec();  
      T_ERROR("Unable to convert the dataset "
               "into the given directory.");
    }

    string strTemp1DTFilename = m_strTempDir + "i3mexport.1dt";
    m_1DTransferFunction->SaveToFile(QString(strTemp1DTFilename.c_str()));
    targetFile = ConvertTF(strTemp1DTFilename, strTargetDir, currentDataset,
                           pleaseWait);
    remove(strTemp1DTFilename.c_str());

    if (targetFile == "") {
      QMessageBox errorMessage;
      errorMessage.setText("Unable to convert the transferfunction "
                           "into the given directory.");
      errorMessage.setIcon(QMessageBox::Critical);
      errorMessage.exec();  
      T_ERROR("Unable to convert the transferfunction "
               "into the given directory.");
    }
  } else {
    QMessageBox errorMessage;
    errorMessage.setText("ImageVis3D Mobile Device Transfer only supported for UVF datasets.");
    errorMessage.setIcon(QMessageBox::Critical);
    errorMessage.exec();  
    T_ERROR("ImageVis3D Mobile Device Transfer only supported for UVF datasets.");
  }
}
