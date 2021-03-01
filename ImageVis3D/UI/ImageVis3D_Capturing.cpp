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

#include <sstream>
#include <QtWidgets/QFileDialog>
#include <QtWidgets/QInputDialog>
#include <QtCore/QSettings>
#include <QtCore/QTimer>
#include "ImageVis3D.h"
#include "../Tuvok/Basics/SysTools.h"
#include "../Tuvok/Controller/Controller.h"

#include "PleaseWait.h"
#include "MIPRotDialog.h"

using namespace std;
using namespace tuvok;

void MainWindow::CaptureFrame() {
  if (m_pActiveRenderWin) {
    if (!CaptureFrame(lineEditCaptureFile->text().toStdWString())) {
      QString msg = tr("Error writing image file %1").arg(lineEditCaptureFile->text());
      ShowWarningDialog( tr("Error"), msg);
      T_ERROR("%s", msg.toStdString().data());
    }
  }
}

void MainWindow::CaptureSequence() {
  if (m_pActiveRenderWin) {
    std::wstring strSequenceName;
    if (!CaptureSequence(lineEditCaptureFile->text().toStdWString(),
                         &strSequenceName)){
      QString msg = tr("Error writing image file %1").arg(QString::fromStdWString(strSequenceName));
      ShowWarningDialog( tr("Error"), msg);
      T_ERROR("%s", msg.toStdString().data());
    }
  }
}

bool MainWindow::CaptureFrame(const std::wstring& strTargetName) {
  if (m_pActiveRenderWin) {
    const bool checked = checkBox_PreserveTransparency->isChecked();
    return m_pActiveRenderWin->CaptureFrame(strTargetName, checked);
  } else {
    WARNING("No render window is open!");
    return false;
  }
}

bool MainWindow::CaptureSequence(const std::wstring& strTargetName,
                                 std::wstring* strRealFilename) {
  if (m_pActiveRenderWin) {
    const bool checked = checkBox_PreserveTransparency->isChecked();
    return m_pActiveRenderWin->CaptureSequenceFrame(strTargetName,
                                                    checked,
                                                    strRealFilename);
  } else {
    WARNING("No render window is open!");
    return false;
  }
}

void MainWindow::CaptureRotation() {
  if (m_pActiveRenderWin) {
    assert(m_pActiveRenderWin->GetActiveRenderRegions().size() == 1);

    const LuaClassInstance renderRegion =
      m_pActiveRenderWin->GetActiveRenderRegions()[0];

    QSettings settings;
    int  iNumImages = settings.value("Renderer/ImagesPerRotation", 360).toInt();
    bool bOrthoView = settings.value("Renderer/RotationUseOrtho", true).toBool();
    bool bStereo    = settings.value("Renderer/RotationUseStereo", false).toBool();
    bool bUseLOD    = settings.value("Renderer/RotationUseLOD", true).toBool();
    int iEyeDist    = settings.value("Renderer/RotationEyeDist", 3).toInt();

    bool ok;
    if (m_pActiveRenderWin->IsRegion2D(renderRegion) &&
        m_pActiveRenderWin->GetUseMIP(renderRegion)) {
      MIPRotDialog mipRotDialog(iNumImages, bOrthoView, bStereo, bUseLOD,
                                iEyeDist, this);
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

      } else {
        ok = false;
      }
    } else {
      iNumImages = QInputDialog::getInt(this,
                     tr("How many images to you want to compute?"),
                     tr("How many images to you want to compute:"),
                     iNumImages, 1, 3600, 1, &ok
                   );
    }
    if (!ok) return;

    settings.setValue("Renderer/ImagesPerRotation", iNumImages);

    m_pActiveRenderWin->ToggleHQCaptureMode();

    PleaseWaitDialog pleaseWait(this, Qt::Tool, true);
    QTLabelOut* labelOut = pleaseWait.AttachLabel(&m_MasterController);

    if (m_pActiveRenderWin->IsRegion3D(renderRegion)) {
      pleaseWait.SetText("Capturing a full 360° rotation, please wait  ...");

      int i = 0;
      float fAngle = 0.0f;
      // Kill the timer, and flush the existing event queue.  This ensures any
      // timers that have fired get processed, and we've got nothing scheduled
      // other than the capture.
      m_pRedrawTimer->stop();
      QCoreApplication::processEvents();
      while (i < iNumImages && !pleaseWait.Canceled()) {
        labelOut->SetOutput(true, true, true, false);
        std::ostringstream progress;
        progress << "Processing Image " << i+1 << " of " << iNumImages;
        if (i==0) {
          progress << " (the first image may be slower due to caching)";
        }
        progress << "\n" << static_cast<int>(100.0f*i/iNumImages)
                 << "% completed.";
        MESSAGE("%s", progress.str().c_str());
        labelOut->SetOutput(false, false, false, false);
        fAngle = float(i)/float(iNumImages) * 360.0f;
        m_pActiveRenderWin->SetCaptureRotationAngle(fAngle);

        m_pActiveRenderWin->SetRendererTarget(AbstrRenderer::RT_CAPTURE);
        while(m_pActiveRenderWin->RendererCheckForRedraw() &&
              !pleaseWait.Canceled()) {
          unsigned sframes = m_pActiveRenderWin->GetCurrentSubFrameCount();
          unsigned sframe = m_pActiveRenderWin->GetWorkingSubFrame();
          unsigned bricks =  m_pActiveRenderWin->GetCurrentBrickCount();
          unsigned brick =  m_pActiveRenderWin->GetWorkingBrick();
          unsigned lod = m_pActiveRenderWin->GetMinLODIndex();
          SetRenderProgressAnUpdateInfo(sframes, sframe, bricks, brick, lod,
                                        m_pActiveRenderWin);
          QCoreApplication::processEvents();
          m_pActiveRenderWin->UpdateWindow();
        }
        if(!pleaseWait.Canceled()) {
          wstring strSequenceName;
          if (!m_pActiveRenderWin->CaptureSequenceFrame(
              lineEditCaptureFile->text().toStdWString(),
              checkBox_PreserveTransparency->isChecked(), &strSequenceName)) {
            QString msg = tr("Error writing image file %1").
                             arg(QString::fromStdWString(strSequenceName));
            ShowWarningDialog(tr("Error"), msg);
            T_ERROR("%s", msg.toStdString().data());
            break;
          }
          i++;
          // Make sure what we've just rendered gets blitted to the RW; user
          // should visually see the progress of the rotation.
          m_pActiveRenderWin->UpdateWindow();
        }
      }
      
    } else {
      if (m_pActiveRenderWin->GetUseMIP(renderRegion)) {
        bool bReUse = true;
        int iReUseOffset = 0;
        wstring strImageFilename = lineEditCaptureFile->text().toStdWString();
        std::vector<std::wstring> vstrLeftEyeImageVector(iNumImages);
        std::vector<std::wstring> vstrRightEyeImageVector(iNumImages);
        if (bStereo) {
          double fDegreePerImage = 360.0/iNumImages;
          iReUseOffset = int(iEyeDist/fDegreePerImage);
          bReUse = (iReUseOffset == iEyeDist/fDegreePerImage);

          if (bReUse)
            strImageFilename = SysTools::AppendFilename(strImageFilename,L"_LR");
          else
            strImageFilename = SysTools::AppendFilename(strImageFilename,L"_L");
        }

        pleaseWait.SetText("Capturing a full 360° MIP rotation,"
                           "please wait  ...");
        float fAngle = 0.0f;
        for (int i = 0;i<iNumImages && !pleaseWait.Canceled();i++) {
          labelOut->SetOutput(true, true, true, false);
          if (bStereo) {
            std::ostringstream progress;
            progress << "Phase 1 of 3: "
                     << static_cast<int>(100.0f*i/iNumImages) << "% completed."
                     << "\nProcessing Image " << i+1 << " of " << iNumImages;
            if(i==0) {
              progress << " (the first image may be slower due to caching)";
            }
            MESSAGE("%s", progress.str().c_str());
          } else {
            std::ostringstream progress;
            progress << static_cast<int>(100.0f*i/iNumImages) << "% completed"
                     << "\nProcessing Image " << i+1 << iNumImages;
            if(i==0) {
              progress << " (the first image may be slower due to caching)";
            }
            MESSAGE("%s", progress.str().c_str());
          }
          labelOut->SetOutput(false, false, false, false);

          fAngle = float(i)/float(iNumImages) * 360.0f;
          std::wstring strSequenceName;

          if (!m_pActiveRenderWin->CaptureMIPFrame(
                  strImageFilename, fAngle, bOrthoView, i==(iNumImages-1),
                  bUseLOD, checkBox_PreserveTransparency->isChecked(),
                  &strSequenceName)) {
            QString msg = tr("Error writing image file %1.").
                          arg(QString::fromStdWString(strSequenceName));
            ShowWarningDialog( tr("Error"), msg);
            T_ERROR("%s", msg.toStdString().data());
            break;
          }

          if (bStereo) {
            vstrLeftEyeImageVector[i] = strSequenceName;
            if (bReUse) {
              vstrRightEyeImageVector[(i+iReUseOffset)%iNumImages] =
                strSequenceName;
            } else {
              fAngle -= 3.0f;
              wstring strImageFilenameRight =
                SysTools::AppendFilename(lineEditCaptureFile->text().
                                                            toStdWString(),L"_R");
              if (!m_pActiveRenderWin->CaptureMIPFrame(
                    strImageFilenameRight, fAngle, bOrthoView,
                    i==(iNumImages-1), bUseLOD,
                    checkBox_PreserveTransparency->isChecked(),
                    &strSequenceName)) {
                QString msg = tr("Error writing image file %1.").
                              arg(QString::fromStdWString(strImageFilenameRight));
                ShowWarningDialog( tr("Error"), msg);
                T_ERROR("%s", msg.toStdString().data());
                break;
              }
              vstrRightEyeImageVector[i] = strSequenceName;
            }
          }
        }

        if (!pleaseWait.Canceled() &&
            m_pActiveRenderWin->GetUseMIP(renderRegion) &&
            bStereo) {
          labelOut->SetOutput(true, true, true, false);

          for (size_t i = 0;i<vstrRightEyeImageVector.size();i++) {
            std::wstring strSourceL = vstrLeftEyeImageVector[i];
            std::wstring strSourceR = vstrRightEyeImageVector[i];
            std::wstring strTarget  = SysTools::FindNextSequenceName(
              lineEditCaptureFile->text().toStdWString()
            );

            MESSAGE("Phase 2 of 3: %u%% completed\nCreating stereo image "
                    "%s from %s and %s\nProcessing Image %u of %i",
                    static_cast<unsigned>(100.0f*i/iNumImages),
                    SysTools::toNarrow(SysTools::GetFilename(strTarget)).c_str(),
                    SysTools::toNarrow(SysTools::GetFilename(strSourceL)).c_str(),
                    SysTools::toNarrow(SysTools::GetFilename(strSourceR)).c_str(),
                    static_cast<unsigned>(i+1), iNumImages);


            QImage imageLeft(QString::fromStdWString(strSourceL));
            QImage imageRight(QString::fromStdWString(strSourceR));


            for (int y = 0;y<imageLeft.height();y++) {
                for (int x = 0;x<imageLeft.width();x++) {
                  QRgb pixelLeft  = imageLeft.pixel(x,y);
                  QRgb pixelRight = imageRight.pixel(x,y);

                  int iGrayLeft  = static_cast<int>(qRed(pixelLeft)  * 0.3f +
                                                    qGreen(pixelLeft)  * 0.59f +
                                                    qBlue(pixelLeft)  * 0.11f);
                  int iGrayRight = static_cast<int>(qRed(pixelRight) * 0.3f +
                                                    qGreen(pixelRight) * 0.59f +
                                                    qBlue(pixelRight) * 0.11f);

                  QRgb pixelStereo = qRgba(iGrayLeft,
                                          iGrayRight/2,
                                          iGrayRight,
                                          255);

                  imageRight.setPixel(x,y,pixelStereo);
                }
            }


            imageRight.save(QString::fromStdWString(strTarget));
          }
          for (size_t i = 0;i<vstrRightEyeImageVector.size();i++) {
            MESSAGE("Phase 3 of 3: %u%% completed\nCleanup\nProcessing Image "
                    "%u of %i", static_cast<unsigned>(100.0f*i/iNumImages),
                    static_cast<unsigned>(i+1), iNumImages);
            SysTools::RemoveFile(vstrRightEyeImageVector[i]);
            if (SysTools::FileExists(vstrLeftEyeImageVector[i])) {
              SysTools::RemoveFile(vstrLeftEyeImageVector[i]);
            }
          }
        }
      } else {
        pleaseWait.SetText("Slicing through the dataset, please wait  ...");
        /// \todo TODO slice capturing
        QString msg = tr("Slice capturing is not implemented yet. Aborting.");
        ShowWarningDialog( tr("Error"), msg);
      }
    }
    m_pActiveRenderWin->ToggleHQCaptureMode();
    m_pRedrawTimer->start(IV3D_TIMER_INTERVAL);
    pleaseWait.close();
    pleaseWait.DetachLabel();
    m_pActiveRenderWin->ScheduleCompleteRedraw();  // to make sure front and backbuffer are valid
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

  QString fileName = QFileDialog::getSaveFileName(this, "Select Image File",
             strLastDir, "All Files (*.*)", &selectedFilter, options);

  if (!fileName.isEmpty()) {
    // add png as the default filetype if the user forgot to enter one
    if (SysTools::GetExt(fileName.toStdWString()) == L"")
      fileName = fileName + ".png";

    settings.setValue("Folders/CaptureFilename",
                      QFileInfo(fileName).absoluteDir().path());
    settings.setValue("Files/CaptureFilename", fileName);
    lineEditCaptureFile->setText(fileName);
  }
}


void MainWindow::PreserveTransparencyChanged() {
  QSettings settings;
  settings.setValue("PreserveTransparency", checkBox_PreserveTransparency->isChecked());
}
