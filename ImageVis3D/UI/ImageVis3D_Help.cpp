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
#include <QMessageBox>
#include <QDesktopServices>

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
                 "%3 %4 %5. Copyright 2008-2014 by the Scientific Computing "
                 "and Imaging (SCI) Institute, and the High Performance "
                 "Computing Group at the University of Duisburg-Essen, Germany.\n\n"
                 "Hilbert Curve implementation copyright 1998, Rice University."
                 "LZ4 - Fast LZ compression algorithm copyright 2011-2012, Yann Collet.")
                    .arg(IV3D_VERSION)
                    .arg(IV3D_VERSION_TYPE)
                    .arg(TUVOK_VERSION)
                    .arg(TUVOK_VERSION_TYPE)
                    .arg(TUVOK_DETAILS);

  AboutDlg d(qstrTitle,qstrText, this);
  connect(&d, SIGNAL(OnlineVideoTutClicked()), this, SLOT(OnlineVideoTut()));
  connect(&d, SIGNAL(OnlineHelpClicked()),     this, SLOT(OnlineHelp()));
  d.exec();
  disconnect(&d, SIGNAL(OnlineVideoTutClicked()), this, SLOT(OnlineVideoTut()));
  disconnect(&d, SIGNAL(OnlineHelpClicked()),     this, SLOT(OnlineHelp()));
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
