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


//!    File   : ImageVis3D_Help.cpp
//!    Author : Jens Krueger
//!             SCI Institute
//!             University of Utah
//!    Date   : January 2009
//
//!    Copyright (C) 2008 SCI Institute

#include "../Tuvok/Basics/StdDefines.h"
#include <algorithm>
#include <array>
#include <cstdio>
#include <fstream>
#include <string>

#include <QtCore/QUrl>
#include <QtCore/QTime>
#include <QtCore/QDate>
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QSettings>
#include <QtCore/QTemporaryFile>
#include <QtCore/QTextStream>
#include <QtWidgets/QMessageBox>
#include <QtGui/QDesktopServices>

#include <QtNetwork/QNetworkReply>
#include <QtNetwork/QAuthenticator>

#include "../Tuvok/Renderer/GPUMemMan/GPUMemMan.h"
#include "ImageVis3D.h"

#ifdef DETECTED_OS_WINDOWS
#   include <ShellAPI.h>
#   include <windows.h>
#   include <io.h>
#   define unlink _unlink
#else
#   include <unistd.h>
#endif

#include "URLDlg.h"
#include "AboutDlg.h"
#include "BugRepDlg.h"
#include "FTPDialog.h"
#include "../Tuvok/Basics/SysTools.h"
#include "../Tuvok/Basics/SystemInfo.h"
#include "../Tuvok/Basics/Appendix.h"
#include "../Tuvok/Controller/Controller.h"


using namespace std;

void MainWindow::ShowAbout()
{

  QString qstrTitle;
  QString qstrText("");



#ifdef _DEBUG
  qstrTitle = tr("ImageVis3D %1 %2 DEBUG VERSION!").arg(IV3D_VERSION).arg(IV3D_VERSION_TYPE);
  qstrText = tr("Warning: this is a DEBUG build!  This version is for "
                "testing only. Some functions may run with dramatically "
                "reduced performance. Please download a release build for "
                "general use.\n\n");
#else
  qstrTitle = tr("ImageVis3D %1").arg(IV3D_VERSION);
#endif
  qstrText += tr("This is the award winning ImageVis3D volume rendering system %1 %2, using the Tuvok render engine "
                 "%3 %4 %5. Copyright 2008-2020 by the Scientific Computing "
                 "and Imaging (SCI) Institute, and the High Performance "
                 "Computing Group at the University of Duisburg-Essen, Germany.\n\n"
                 "Using Qt Version: %6.%7.%8\n"
                 "Hilbert Curve implementation copyright 1998, Rice University."
                 "LZ4 - Fast LZ compression algorithm copyright 2011-2012, Yann Collet.")
                    .arg(IV3D_VERSION)
                    .arg(IV3D_VERSION_TYPE)
                    .arg(TUVOK_VERSION)
                    .arg(TUVOK_VERSION_TYPE)
                    .arg(TUVOK_DETAILS)
                    .arg(QT_VERSION_MAJOR)
                    .arg(QT_VERSION_MINOR)
                    .arg(QT_VERSION_PATCH);

  AboutDlg d(qstrTitle,qstrText, this);
  connect(&d, SIGNAL(CheckUpdatesClicked()),   this, SLOT(CheckForUpdates()));
  connect(&d, SIGNAL(OnlineVideoTutClicked()), this, SLOT(OnlineVideoTut()));
  connect(&d, SIGNAL(OnlineHelpClicked()),     this, SLOT(OnlineHelp()));
  connect(&d, SIGNAL(ReportABugClicked()),     this, SLOT(ReportABug()));
  d.exec();
  disconnect(&d, SIGNAL(CheckUpdatesClicked()),   this, SLOT(CheckForUpdates()));
  disconnect(&d, SIGNAL(OnlineVideoTutClicked()), this, SLOT(OnlineVideoTut()));
  disconnect(&d, SIGNAL(OnlineHelpClicked()),     this, SLOT(OnlineHelp()));
  disconnect(&d, SIGNAL(ReportABugClicked()),     this, SLOT(ReportABug()));
}

void MainWindow::QuietCheckForUpdates() {
  m_bStartupCheck = true;
  // This should really be done in a background thread...
  CheckForUpdatesInternal();
}

void MainWindow::CheckForUpdates() {
  m_bStartupCheck = false;
  CheckForUpdatesInternal();
}


void MainWindow::CheckForUpdatesInternal() {
#ifndef PACKAGE_MANAGER
  // cleanup updatefile, this codepath is taken for instance when the windows firewall blocked an http request
  if (m_pUpdateFile && m_pUpdateFile->isOpen()) {
    m_pUpdateFile->close();

    DeleteUpdateFile();
  }

  QString strUpdateFile = tr("%1/ImageVis3D_UpdateCheck_Temp").arg(QDir::tempPath());
  m_pUpdateFile = new QTemporaryFile(strUpdateFile);
  m_pUpdateFile->open(QIODevice::WriteOnly);

  QString strCompleteFile;
  if (m_bCheckForDevBuilds)
      strCompleteFile = tr("%1%2").arg(UPDATE_NIGHTLY_PATH).arg(UPDATE_VERSION_FILE);
  else
      strCompleteFile = tr("%1%2").arg(UPDATE_VERSION_PATH).arg(UPDATE_VERSION_FILE);

  QUrl url(strCompleteFile);
  m_pHttpReply = m_Http.get(QNetworkRequest(url));
  connect(m_pHttpReply, &QNetworkReply::finished, this, &MainWindow::httpFinished);
  connect(m_pHttpReply, &QIODevice::readyRead, this, &MainWindow::httpReadyRead);


#endif
}

void MainWindow::httpReadyRead() {
    // this slot gets called every time the QNetworkReply has new data.
    // We read all of its new data and write it into the file.
    // That way we use less RAM than when reading it at the finished()
    // signal of the QNetworkReply
    if (m_pUpdateFile && m_pHttpReply)
        m_pUpdateFile->write(m_pHttpReply->readAll());
}

void MainWindow::httpFinished() {
    QFileInfo fi;

    if (m_pUpdateFile) {
        fi.setFile(m_pUpdateFile->fileName());
        m_pUpdateFile->close();
        m_pUpdateFile = nullptr;
    } else {
        return;
    }


  if (!m_bScriptMode) {
    if (m_pHttpReply->error()) {
        QFile::remove(fi.absoluteFilePath());

        if (!m_bStartupCheck)
          ShowInformationDialog( tr("Update Check"), tr("Download failed: %1.") .arg(m_pHttpReply->errorString()));

    } else {
      struct VersionNumber iv3d;
      struct VersionNumber tuvok;
#ifdef IV3D_SVN_VERSION
      size_t iv3d_svn = IV3D_SVN_VERSION;
#else
      size_t iv3d_svn = 0;
#endif
#ifdef TUVOK_SVN_VERSION
      size_t tuvok_svn = TUVOK_SVN_VERSION;
#else
      size_t tuvok_svn = 0;
#endif
      if (GetVersionsFromUpdateFile(fi.absoluteFilePath().toStdWString(),
                                    iv3d, tuvok)) {
        const struct VersionNumber local_iv3d = {
          IV3D_MAJOR, IV3D_MINOR, IV3D_PATCH, iv3d_svn
        };
        const struct VersionNumber local_tuvok = {
          TUVOK_MAJOR, TUVOK_MINOR, TUVOK_PATCH, tuvok_svn
        };
        // First check the release version numbers.
        if(iv3d > local_iv3d || tuvok > local_tuvok) {
          QString qstrMessage = tr("A newer ImageVis3D was found "
                                   "(version %1, %2 Tuvok). "
                                   "You SHOULD download the newer version at").
                                   arg(std::string(iv3d).c_str()).
                                   arg(std::string(tuvok).c_str());
          URLDlg u(tr("Update Check"), qstrMessage, UPDATE_STABLE_PATH, this);
          u.exec();
        } else if(iv3d == local_iv3d && tuvok == local_tuvok) {
          // Then check svn revisions, if the user cares for devbuilds.
#if defined(IV3D_SVN_VERSION) && defined(TUVOK_SVN_VERSION)
          if(m_bCheckForDevBuilds &&
             (local_iv3d.svn < iv3d.svn || local_tuvok.svn < tuvok.svn)) {
            QString qstrMessage = tr("A new SVN devbuild (%1-%2) of "
                                     "ImageVis3D was found. You MAY "
                                     "want to download the newer version at").
                                     arg(std::string(iv3d).c_str()).
                                     arg(std::string(tuvok).c_str());
            QString qstrURL = tr("%1%2").arg(UPDATE_NIGHTLY_PATH).arg(UPDATE_FILE);
            URLDlg u(tr("Update Check"), qstrMessage, qstrURL, this);
            u.exec();
          } else // if() continued after #endif!
#endif
          if (!m_bStartupCheck) {
            ShowInformationDialog(tr("Update Check"),
                                  tr("This is the most current version "
                                     "of ImageVis3D and Tuvok!"));
          }
        }
      } else {
        T_ERROR("Could not parse versions file.");
      }
    }
  }

  if (m_pHttpReply) {
    m_pHttpReply->deleteLater();
    m_pHttpReply = nullptr;
  }
}

void MainWindow::DeleteUpdateFile() {
  if (m_pUpdateFile) {
    m_pUpdateFile->remove();
    delete m_pUpdateFile;
    m_pUpdateFile = NULL;
  }
}

MainWindow::VersionNumber::operator std::string() const {
  std::ostringstream oss;
  oss << this->major << "." << this->minor << "." << this->patch;
  return oss.str();
}
bool MainWindow::VersionNumber::operator==(const VersionNumber &vn) const
{
  return this->major == vn.major &&
         this->minor == vn.minor &&
         this->patch == vn.patch;
}
bool MainWindow::VersionNumber::operator>(const VersionNumber &vn) const
{
  return this->major > vn.major ||
         (this->major == vn.major && this->minor > vn.minor) ||
         (this->major == vn.major && this->minor == vn.minor &&
          this->patch > vn.patch);
}

bool MainWindow::GetVersionsFromUpdateFile(const std::wstring& strFilename,
                                           struct VersionNumber& iv3d,
                                           struct VersionNumber& tuvok) {
  // Version File contains 3 lines:
  // IV3DMAJOR.IV3DMINOR.IV3DPATCH
  // IV3DSVN
  // TUVOKMAJOR.TUVOKMINOR.TUVOKPATCH
  // Example:
  // 3.2.1
  // 29
  // 3.2.1

  string line ="";
  ifstream updateFile(SysTools::toNarrow(strFilename).c_str(),ios::binary);

#define CHECK_EOF() \
  do { \
    if(updateFile.eof()) { updateFile.close(); return false; } \
  } while(0)

  if(!updateFile.is_open()) {
    DeleteUpdateFile();
    return false;
  }

  CHECK_EOF();

  char skip; // use to skip the "." characters in version strings.
  // IV3D version.
  getline(updateFile, line);
  { std::istringstream iss(line);
    iss >> iv3d.major >> skip >> iv3d.minor >> skip >> iv3d.patch;
    MESSAGE("read iv3d vnumber: %u.%u.%u",
            static_cast<unsigned int>(iv3d.major),
            static_cast<unsigned int>(iv3d.minor),
            static_cast<unsigned int>(iv3d.patch));
  }
  CHECK_EOF();
  // IV3D svn revision.
  getline(updateFile, line);
  { std::istringstream iss(line);
    iss >> iv3d.svn;
  }
  CHECK_EOF();

  // Tuvok version
  getline(updateFile, line);
  { std::istringstream iss(line);
    iss >> tuvok.major >> skip >> tuvok.minor >> skip >> tuvok.patch;
    MESSAGE("read tuvok vnumber: %u.%u.%u",
            static_cast<unsigned int>(tuvok.major),
            static_cast<unsigned int>(tuvok.minor),
            static_cast<unsigned int>(tuvok.patch));
  }
  CHECK_EOF();
  // Tuvok svn revision
  getline(updateFile, line);
  { std::istringstream iss(line);
    iss >> tuvok.svn;
  }

  updateFile.close();
  DeleteUpdateFile();

  return true;
}

void MainWindow::UploadLogToServer() {
  QFile* pFTPTempFile =new QFile("SubmittedDebugOutput.txt");

  if (!pFTPTempFile->open(QIODevice::WriteOnly | QIODevice::Text)) {
     ShowInformationDialog("Log creation failed",
                           "Could not create a log file to submit.");
     return;
  }
  QTextStream outstream(pFTPTempFile);

  for (int i = 0;i<listWidget_DebugOut->count();i++)
    outstream << listWidget_DebugOut->item(i)->text() << "\n";
  pFTPTempFile->close();

  const wstring strSourceName = pFTPTempFile->fileName().toStdWString();

  delete pFTPTempFile;

  QString qstrID = GenUniqueName(L"DebugOut",L"txt");
  FtpTransfer(strSourceName, qstrID.toStdWString());
}


QString MainWindow::GenUniqueName(const std::wstring& strPrefix, const std::wstring& strExt) {
  return tr("%1_%2_%3.%4").arg(QString::fromStdWString(strPrefix)).arg(QTime::currentTime().toString()).arg(QDate::currentDate().toString()).arg(QString::fromStdWString(strExt));
}

bool MainWindow::FtpTransfer(const std::wstring& strSource, const std::wstring& strDest,
                             bool bDeleteSource) {
  if (!m_bFTPFinished) return false;
  m_bFTPFinished = true;
  m_strFTPTempFile = strSource;

  if (m_pFTPDialog) {
    disconnect(m_pFTPDialog, 0,0,0);
    delete m_pFTPDialog;
  }

  m_bFTPDeleteSource = bDeleteSource;
  std::wstring strFullDest = std::wstring(DEBUG_DUMP_PATH) + strDest;

  m_pFTPDialog = new FTPDialog(strSource, std::wstring(DEBUG_DUMP_SERVER) ,strFullDest, this);

  connect(m_pFTPDialog, SIGNAL(TransferFailure()), this, SLOT(FtpFail()));
  connect(m_pFTPDialog, SIGNAL(TransferSuccess()), this, SLOT(FtpSuccess()));

  m_pFTPDialog->Start();

  return true;
}

void MainWindow::FtpFail() {
  if (SysTools::FileExists(m_strFTPTempFile) && m_bFTPDeleteSource) SysTools::RemoveFile(m_strFTPTempFile);
  ShowInformationDialog("Transfer failed", "Transfer failed");
  m_bFTPFinished = true;
}

void MainWindow::FtpSuccess() {
  if (SysTools::FileExists(m_strFTPTempFile) && m_bFTPDeleteSource) SysTools::RemoveFile(m_strFTPTempFile);
  ShowInformationDialog("Transfer successfull", "Transfer successfull");
  m_bFTPFinished = true;
}

void MainWindow::OnlineHelp() {
  QDesktopServices::openUrl(QString(HELP_URL));
}

bool readable(const std::string& f) {
  return QFile(QString(f.c_str())).permissions() & QFile::ReadUser;
}

void MainWindow::OpenManual() {
#ifdef DETECTED_OS_WINDOWS
  ShellExecuteA(NULL, "open", MANUAL_NAME, NULL,NULL,SW_SHOWDEFAULT);
#elif defined(DETECTED_OS_APPLE)
  string manualOpenCall = "open " + SysTools::GetFromResourceOnMac(MANUAL_NAME);
  system(manualOpenCall.c_str());
#else
  // This path assumes at least "POSIX", and the /proc FS is linux-specific.
  // Technically things should still work even if /proc doesn't exist.

  /// Find out where our binary lives.  The manual is placed in the same
  /// directory in the case of the binary tarballs we provide.
  std::vector<std::string> paths; paths.reserve(4);
  char linkbuf[1024];
  memset(linkbuf, 0, 1024);
  if(readlink("/proc/self/exe", linkbuf, 1024) == -1) {
    T_ERROR("Error reading /proc/self/exe; ignoring binary directory while "
            "searching for manual.");
  } else {
    paths.push_back(QFileInfo(linkbuf).absolutePath().toStdString() +
                    MANUAL_NAME);
  }
  paths.push_back(std::string("/usr/share/doc/imagevis3d/") + MANUAL_NAME);
  paths.push_back(std::string("/usr/local/share/doc/imagevis3d/") +
                  MANUAL_NAME);
  paths.push_back(MANUAL_NAME);
  std::vector<std::string>::const_iterator found = find_if(paths.begin(),
                                                           paths.end(),
                                                           readable);
  if(found == paths.end()) {
    T_ERROR("Could not find manual...");
    QMessageBox::critical(this, "Manual Not Found",
                          "Could not find the local manual.  This is probably "
                          "a packaging/distribution error -- please use the "
                          "'Report an Issue' feature and mention how you "
                          "obtained this ImageVis3D binary.  Thanks!\n\n"
                          "As a workaround, use the 'Open Online Help' "
                          "option to download the latest version of the "
                          "manual.");
    return;
  }
  MESSAGE("Manual found at: %s", found->c_str());

  // Now, figure out how to load the PDF.
  QUrl uri;
  {
    std::ostringstream u;
    u << *found;
    QString pth = QFileInfo(u.str().c_str()).absoluteFilePath();
    uri.setUrl(pth);
    MESSAGE("uri: %s", pth.toStdString().c_str());
  }
  if(!QDesktopServices::openUrl(uri)) {
    MESSAGE("Manual open failed, relying on manual method.");
    typedef std::array<std::string, 3> progvec;
    progvec viewers = {{ "evince", "xpdf", "acroread" }};
    // Now iterate through each of those programs until one of them
    // successfully launches.
    for(progvec::const_iterator prog = viewers.begin(); prog != viewers.end();
        ++prog) {
      char manual[1024];
      snprintf(manual, 1024, "%s %s", prog->c_str(), found->c_str());
      if(system(manual) != -1) {
        break;
      }
    }
  }
#endif
}

void MainWindow::OnlineVideoTut() {
  QDesktopServices::openUrl(QString(TUTORIAL_URL));
}

void MainWindow::GetExampleData() {
  QDesktopServices::openUrl(QString(DEMO_DATA_URL));
}

void MainWindow::CloseWelcome() {
  if (m_pWelcomeDialog->ShowAtStartup() != m_bShowWelcomeScreen) {
    QSettings settings;
    settings.setValue("UI/ShowWelcomeScreen", m_pWelcomeDialog->ShowAtStartup());
  }
}

void MainWindow::ReportABug() {
  ReportABug(L"");
}

void MainWindow::ReportABug(const wstring& strFile) {
  BugRepDlg b(this, Qt::Tool, strFile);

  QSettings settings;
  settings.beginGroup("BugReport");
  b.SetSubmitSysinfo(settings.value("SubmitSysinfo", true).toBool());
  b.SetSubmitLog(settings.value("SubmitLog", true).toBool());
  b.SetUsername(string(settings.value("Username", "").toString().toStdString()));
  b.SetUserMail(string(settings.value("UserMail", "").toString().toStdString()));

  while(b.exec() != QDialog::Rejected) {
    {
      std::string err;
      if(!b.Validate(err)) {
        err = "An error occurred while validating the bug report.\n\n" + err;
        QMessageBox::critical(this, "Validation Error", err.c_str());
        continue;
      }
    }
    settings.setValue("SubmitSysinfo", b.SubmitSysinfo());
    settings.setValue("SubmitLog", b.SubmitLog());
    settings.setValue("Username", b.GetUsername().c_str());
    settings.setValue("UserMail", b.GetUserMail().c_str());

    // first create the report textfile
    ofstream reportFile("bugreport.txt");
    if (!reportFile.is_open()) {
      ShowWarningDialog("Warning", "Unable to create bugreport.txt, aborting.");
      return;
    }

    string strDate(QDate::currentDate().toString().toStdString());
    string strTime(QTime::currentTime().toString().toStdString());
    reportFile << "Issue Report " << strDate << "  " << strTime << endl << endl << endl;

    reportFile << "Tuvok Version:" << TUVOK_VERSION << " "
               << TUVOK_VERSION_TYPE << " " << TUVOK_DETAILS;
#ifdef TUVOK_SVN_VERSION
    reportFile << " SVN Version:" << int(TUVOK_SVN_VERSION);
#endif
    reportFile << std::endl << "ImageVis3D Version:" << IV3D_VERSION
               << " " << IV3D_VERSION_TYPE;
#ifdef IV3D_SVN_VERSION
    reportFile << " SVN Version:" << int(IV3D_SVN_VERSION);
#endif
    reportFile << endl << "QT Version:" << QT_VERSION_STR << endl;
    reportFile << "This is a "<< m_MasterController.MemMan()->GetBitWidthMem() << "bit build." << endl;

    if (b.GetUsername() != "") reportFile << "User:" << b.GetUsername() << endl;
    if (b.GetUserMail() != "") reportFile << "Email:" << b.GetUserMail() << endl;

    reportFile << endl << endl << "Description:" << endl << b.GetDescription() << endl;

    if (b.SubmitSysinfo() ) {
      reportFile << endl << endl << "Memory info:" << endl;

      const uint64_t mb = 1024*1024;
      reportFile << "CPU Memory: Total "
                 << m_MasterController.MemMan()->GetCPUMem()/mb
                 << " MB, Usable "
                 << m_MasterController.SysInfo()->GetMaxUsableCPUMem()/mb
                 << " MB" << endl;
      reportFile << "    Used: "
                 << m_MasterController.MemMan()->GetAllocatedCPUMem()/mb
                 << " MB ("
                 << m_MasterController.MemMan()->GetAllocatedCPUMem()
                 << " Bytes)" << endl;

      if (m_MasterController.MemMan()->GetAllocatedCPUMem() <
          m_MasterController.MemMan()->GetCPUMem()) {
        reportFile << "    Available: "
                   << (m_MasterController.MemMan()->GetCPUMem() -
                       m_MasterController.MemMan()->GetAllocatedCPUMem())/mb
                   << "MB" << endl;
      }

      reportFile << "GPU Memory: Total "
                 << m_MasterController.MemMan()->GetGPUMem()/mb
                 << " MB, Usable "
                 << m_MasterController.SysInfo()->GetMaxUsableGPUMem()/mb
                 << " MB" << endl;
      reportFile << "    Used: "
                 << m_MasterController.MemMan()->GetAllocatedGPUMem()/mb
                 << " MB ("
                 << m_MasterController.MemMan()->GetAllocatedGPUMem()
                 << " Bytes)" << endl;

      if (m_MasterController.MemMan()->GetAllocatedGPUMem() <
          m_MasterController.MemMan()->GetGPUMem()) {
        reportFile << "    Available: "
                   << (m_MasterController.MemMan()->GetGPUMem() -
                       m_MasterController.MemMan()->GetAllocatedGPUMem())/mb
                   <<" MB" << endl;
      }

      reportFile << endl << endl << "GPU info:" << endl;

      #if defined(_WIN32) && defined(USE_DIRECTX)
        if (DynamicDX::IsInitialized())
          reportFile << "Direct3DX10 Version " << DynamicDX::GetD3DX10Version() << endl;
        else
          reportFile << "DirectX 10 not initialzed" << endl;
      #endif

      if (RenderWindow::GetVendorString() == "") {
        reportFile << "No GL-info discovered yet" << endl;
      } else {
        reportFile << RenderWindow::GetVendorString().c_str() << endl;
        reportFile << "Maximum 3D texture size "
                   << RenderWindow::GetMax3DTexDims() << endl;
        reportFile << "Supported GL extensions:" << endl;
        reportFile << RenderWindowGL::GetExtString() << endl;
      }
    }

    if (b.SubmitLog()) {
      reportFile << endl << endl << "Debug Log:" << endl;
      for (int i = 0;i<listWidget_DebugOut->count();i++) {
        string line(listWidget_DebugOut->item(i)->text().toStdString());
        reportFile << "   " << line << endl;
      }
    }

    reportFile.close();

    // combine everything into a single file
    vector<string> vFiles = b.GetDataFilenames();
    vFiles.push_back("bugreport.txt");

    Appendix a("report.apx", vFiles);

    remove("bugreport.txt");
    if (!a.IsOK()) {
      ShowWarningDialog("Warning", "Unable to create report file report.apx,"
                                   " aborting.");
      return;
    }

    // compress that single file
    /// \todo Tom add compressor here

    QString qstrID = tr("ErrorReport_%1_%2.apx").arg(QTime::currentTime().toString()).arg(QDate::currentDate().toString());
    if (!FtpTransfer(L"report.apx", qstrID.toStdWString(), true)) {
      remove("report.apx");
      ShowWarningDialog("Warning", "Another FTP transfer is still in progress, aborting.");
      return;
    }
    return;
  }
}
