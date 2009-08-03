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


//!    File   : ImageVis3D_Capturing.cpp
//!    Author : Jens Krueger
//!             SCI Institute
//!             University of Utah
//!    Date   : September 2008
//
//!    Copyright (C) 2008 SCI Institute

#include "ImageVis3D.h"
#include "../Tuvok/Basics/SysTools.h"
#include <QtGui/QFileDialog>
#include <QtGui/QInputDialog>
#include <QtCore/QSettings>

#include "PleaseWait.h"
#include "MIPRotDialog.h"

using namespace std;


void MainWindow::CaptureFrame() {
  if (m_pActiveRenderWin) {
    if (!CaptureFrame(lineEditCaptureFile->text().toStdString())) {
      QString msg = tr("Error writing image file %1").arg(lineEditCaptureFile->text());
      ShowWarningDialog( tr("Error"), msg);
      m_MasterController.DebugOut()->Error("MainWindow::CaptureFrame", msg.toAscii());
    }
  }
}

void MainWindow::CaptureSequence() {
  if (m_pActiveRenderWin) {
    string strSequenceName;
    if (!CaptureSequence(lineEditCaptureFile->text().toStdString(), &strSequenceName)){
      QString msg = tr("Error writing image file %1").arg(strSequenceName.c_str());
      ShowWarningDialog( tr("Error"), msg);
      m_MasterController.DebugOut()->Error("MainWindow::CaptureSequence", msg.toAscii());
    }
  }
}

bool MainWindow::CaptureFrame(const std::string& strTargetName) {
  if (m_pActiveRenderWin)
    return m_pActiveRenderWin->CaptureFrame(strTargetName, checkBox_PreserveTransparency->isChecked());
  else {
    m_MasterController.DebugOut()->Warning("MainWindow::CaptureFrame",
                                           "No render window is open!");
    return false;
  }
}

bool MainWindow::CaptureSequence(const std::string& strTargetName, std::string* strRealFilename) {
  if (m_pActiveRenderWin)
    return m_pActiveRenderWin->CaptureSequenceFrame(strTargetName, checkBox_PreserveTransparency->isChecked(), strRealFilename);
  else {
    m_MasterController.DebugOut()->Warning("MainWindow::CaptureSequence",
                                           "No render window is open!");
    return false;
  }
}

void MainWindow::CaptureRotation() {
  if (m_pActiveRenderWin) {

    AbstrRenderer::EWindowMode eWindowMode = m_pActiveRenderWin->GetRenderer()->GetFullWindowmode();

    QSettings settings;
    int  iNumImages = settings.value("Renderer/ImagesPerRotation", 360).toInt();
    bool bOrthoView = settings.value("Renderer/RotationUseOrtho", true).toBool();
    bool bStereo    = settings.value("Renderer/RotationUseStereo", false).toBool();
    bool bUseLOD    = settings.value("Renderer/RotationUseLOD", true).toBool();
    int iEyeDist    = settings.value("Renderer/RotationEyeDist", 3).toInt();

    bool ok;
    if (m_pActiveRenderWin->GetRenderer()->GetUseMIP(eWindowMode))  {
      MIPRotDialog mipRotDialog(iNumImages, bOrthoView, bStereo, bUseLOD, iEyeDist, this);
      if (mipRotDialog.exec() == QDialog::Accepted) {
        ok = true;
        iNumImages = mipRotDialog.GetNumImages();
        bOrthoView = mipRotDialog.GetUseOrtho();
        bStereo    = mipRotDialog.GetUseStereo();
        bUseLOD    = mipRotDialog.GetUseLOD();
        iEyeDist   = mipRotDialog.GetEyeDist();

        settings.setValue("Renderer/RotationUseOrtho", bOrthoView);
        settings.setValue("Renderer/RotationUseStereo", bStereo);
        settings.setValue("Renderer/RotationUseLOD", bUseLOD);
        settings.setValue("Renderer/RotationEyeDist", iEyeDist);

      } else ok = false;
    } else {
      iNumImages = QInputDialog::getInteger(this,
                                            tr("How many images to you want to compute?"),
                                            tr("How many images to you want to compute:"), iNumImages, 1, 3600, 1, &ok);
    }
    if (!ok) return;

    settings.setValue("Renderer/ImagesPerRotation", iNumImages);

    m_pActiveRenderWin->ToggleHQCaptureMode();

    PleaseWaitDialog pleaseWait(this);
    QTLabelOut* labelOut = pleaseWait.AttachLabel(&m_MasterController);

    if (eWindowMode == AbstrRenderer::WM_3D)  {
      pleaseWait.SetText("Capturing a full 360° rotation, please wait  ...");

      int i = 0;
      float fAngle = 0.0f;
      while (i < iNumImages) {
        labelOut->SetOutput(true, true, true, false);
        if (i==0)
          m_MasterController.DebugOut()->Message("MainWindow::CaptureRotation", "Processing Image %i of %i (the first image may be slower due to caching)\n%i%% completed",i+1,iNumImages,int(100*float(i)/float(iNumImages)) );
        else
          m_MasterController.DebugOut()->Message("MainWindow::CaptureRotation", "Processing Image %i of %i\n%i%% completed",i+1,iNumImages,int(100*float(i)/float(iNumImages)) );
        labelOut->SetOutput(false, false, false, false);
        fAngle = float(i)/float(iNumImages) * 360.0f;
        m_pActiveRenderWin->SetCaptureRotationAngle(fAngle);
        string strSequenceName;
        if (!m_pActiveRenderWin->CaptureSequenceFrame(lineEditCaptureFile->text().toStdString(), checkBox_PreserveTransparency->isChecked(), &strSequenceName)) {
          QString msg = tr("Error writing image file %1").arg(strSequenceName.c_str());
          ShowWarningDialog( tr("Error"), msg);
          m_MasterController.DebugOut()->Error("MainWindow::CaptureRotation", msg.toAscii());
          break;
        }
        i++;
      }
    } else {
      if (m_pActiveRenderWin->GetRenderer()->GetUseMIP(eWindowMode))  {

        bool bReUse = true;
        int iReUseOffset = 0;
        string strImageFilename = lineEditCaptureFile->text().toStdString();
        vector<string> vstrLeftEyeImageVector(iNumImages);
        vector<string> vstrRightEyeImageVector(iNumImages);
        if (bStereo) {

          double fDegreePerImage = 360.0/iNumImages;
          iReUseOffset = int(iEyeDist/fDegreePerImage);
          bReUse = (iReUseOffset == iEyeDist/fDegreePerImage);

          if (bReUse)
            strImageFilename = SysTools::AppendFilename(strImageFilename,"_LR");
          else
            strImageFilename = SysTools::AppendFilename(strImageFilename,"_L");
        }

        pleaseWait.SetText("Capturing a full 360° MIP rotation, please wait  ...");
        float fAngle = 0.0f;
        for (int i = 0;i<iNumImages;i++) {
          labelOut->SetOutput(true, true, true, false);
          if (bStereo) {
            if (i==0)
              m_MasterController.DebugOut()->Message("MainWindow::CaptureRotation", "Phase 1 of 3: %i%% completed\nProcessing Image %i of %i (the first image may be slower due to caching)",int(100*float(i)/float(iNumImages)),i+1,iNumImages );
            else
              m_MasterController.DebugOut()->Message("MainWindow::CaptureRotation", "Phase 1 of 3: %i%% completed\nProcessing Image %i of %i",int(100*float(i)/float(iNumImages)),i+1,iNumImages);
          } else {
            if (i==0)
              m_MasterController.DebugOut()->Message("MainWindow::CaptureRotation", "%i%% completed\nProcessing Image %i of %i (the first image may be slower due to caching)",int(100*float(i)/float(iNumImages)),i+1,iNumImages );
            else
              m_MasterController.DebugOut()->Message("MainWindow::CaptureRotation", "%i%% completed\nProcessing Image %i of %i",int(100*float(i)/float(iNumImages)),i+1,iNumImages);
          }
          labelOut->SetOutput(false, false, false, false);

          fAngle = float(i)/float(iNumImages) * 360.0f;
          string strSequenceName;

          if (!m_pActiveRenderWin->CaptureMIPFrame(strImageFilename, fAngle, bOrthoView, i==(iNumImages-1), bUseLOD, checkBox_PreserveTransparency->isChecked(), &strSequenceName)) {
            QString msg = tr("Error writing image file %1.").arg(strSequenceName.c_str());
            ShowWarningDialog( tr("Error"), msg);
            m_MasterController.DebugOut()->Error("MainWindow::CaptureRotation", msg.toAscii());
            break;
          }

          if (bStereo) {
            vstrLeftEyeImageVector[i] = strSequenceName;
            if (bReUse) {
              vstrRightEyeImageVector[(i+iReUseOffset)%iNumImages] = strSequenceName;
            } else {
              fAngle -= 3.0f;
              string strImageFilenameRight = SysTools::AppendFilename(lineEditCaptureFile->text().toStdString(),"_R");
              if (!m_pActiveRenderWin->CaptureMIPFrame(strImageFilenameRight, fAngle, bOrthoView, bUseLOD, i==(iNumImages-1), checkBox_PreserveTransparency->isChecked(), &strSequenceName)) {
                QString msg = tr("Error writing image file %1.").arg(strImageFilenameRight.c_str());
                ShowWarningDialog( tr("Error"), msg);
                m_MasterController.DebugOut()->Error("MainWindow::CaptureRotation", msg.toAscii());
                break;
              }
              vstrRightEyeImageVector[i] = strSequenceName;
            }
          }
        }

        if (m_pActiveRenderWin->GetRenderer()->GetUseMIP(eWindowMode) && bStereo) {
          labelOut->SetOutput(true, true, true, false);

          for (size_t i = 0;i<vstrRightEyeImageVector.size();i++) {
            string strSourceL = vstrLeftEyeImageVector[i];
            string strSourceR = vstrRightEyeImageVector[i];
            string strTarget  = SysTools::FindNextSequenceName(lineEditCaptureFile->text().toStdString());

            m_MasterController.DebugOut()->Message("MainWindow::CaptureRotation", "Phase 2 of 3: %i%% completed\nCreating stereo image %s from %s and %s \nProcessing Image %i of %i", int(100*float(i)/float(iNumImages)), SysTools::GetFilename(strTarget).c_str(), SysTools::GetFilename(strSourceL).c_str(), SysTools::GetFilename(strSourceR).c_str(), i+1,iNumImages );

            QImage imageLeft(strSourceL.c_str());
            QImage imageRight(strSourceR.c_str());

            for (int y = 0;y<imageLeft.height();y++) {
                for (int x = 0;x<imageLeft.width();x++) {
                  QRgb pixelLeft  = imageLeft.pixel(x,y);
                  QRgb pixelRight = imageRight.pixel(x,y);

                  int iGrayLeft  = int(qRed(pixelLeft)  * 0.3f + qGreen(pixelLeft)  * 0.59f + qBlue(pixelLeft)  * 0.11f);
                  int iGrayRight = int(qRed(pixelRight) * 0.3f + qGreen(pixelRight) * 0.59f + qBlue(pixelRight) * 0.11f);


                  QRgb pixelStereo = qRgba(iGrayLeft,
                                          iGrayRight/2,
                                          iGrayRight,
                                          255);

                  imageRight.setPixel(x,y,pixelStereo);
                }
            }


            imageRight.save(strTarget.c_str());
          }
          for (size_t i = 0;i<vstrRightEyeImageVector.size();i++) {
            m_MasterController.DebugOut()->Message("MainWindow::CaptureRotation", "Phase 3 of 3: %i%% completed\nCleanup\nProcessing Image %i of %i",int(100*float(i)/float(iNumImages)),i+1,iNumImages );
            remove(vstrRightEyeImageVector[i].c_str());
            if (SysTools::FileExists(vstrLeftEyeImageVector[i])) remove(vstrLeftEyeImageVector[i].c_str());
          }
        }
      } else {
        pleaseWait.SetText("Slicing trougth the dataset, please wait  ...");
        /// \todo TODO slice capturing
        QString msg = tr("Slice Capturing is not implemented yet. Aborting.");
        ShowWarningDialog( tr("Error"), msg);
      }
    }
    m_pActiveRenderWin->ToggleHQCaptureMode();
    pleaseWait.close();
    pleaseWait.DetachLabel();
    m_pActiveRenderWin->GetRenderer()->ScheduleCompleteRedraw();  // to make sure front and backbuffer are valid
  }
}

void MainWindow::SetCaptureFilename() {
  QFileDialog::Options options;
#ifdef DETECTED_OS_APPLE
  options |= QFileDialog::DontUseNativeDialog;
#endif
  QString selectedFilter;

  QSettings settings;
  QString strLastDir = settings.value("Folders/CaptureFilename", ".").toString();

  QString fileName = QFileDialog::getSaveFileName(this,"Select Image File", strLastDir,
             "All Files (*.*)",&selectedFilter, options);

  if (!fileName.isEmpty()) {
    // add png as the default filetype if the user forgot to enter one
    if (SysTools::GetExt(fileName.toStdString()) == "")
      fileName = fileName + ".png";

    settings.setValue("Folders/CaptureFilename", QFileInfo(fileName).absoluteDir().path());
    settings.setValue("Files/CaptureFilename", fileName);
    lineEditCaptureFile->setText(fileName);
  }
}


void MainWindow::PreserveTransparencyChanged() {
  QSettings settings;
  settings.setValue("PreserveTransparency", checkBox_PreserveTransparency->isChecked());
}
