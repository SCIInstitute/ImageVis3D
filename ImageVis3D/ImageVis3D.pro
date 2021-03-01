TEMPLATE          = app
win32:TEMPLATE    = vcapp
CONFIG           += exceptions largefile qt rtti static stl uic warn_on
macx-clang:DEFINES     += QT_MAC_USE_COCOA=1
TARGET            = ../Build/ImageVis3D
macx-clang {
  DESTDIR         = ../Build
  TARGET          = ImageVis3D
}
RCC_DIR           = ../Build/rcc
OBJECTS_DIR       = ../Build/objects
UI_DIR            = UI/AutoGen
MOC_DIR           = UI/AutoGen
incpath           = . ../Tuvok/Basics ../Tuvok/Controller ../Tuvok/DebugOut
incpath          += ../Tuvok/IO ../Tuvok/IO/exception ../Tuvok/Renderer
incpath          += ../Tuvok
incpath          += DebugOut UI UI/AutoGen
incpath          += ../Tuvok/IO/3rdParty/boost ../Tuvok/3rdParty/GLEW
DEPENDPATH       += $$incpath
INCLUDEPATH      += $$incpath
#INCLUDEPATH      += . ../Tuvok/IO/3rdParty/boost ../Tuvok/3rdParty/GLEW ../Tuvok
QT               += opengl network
QMAKE_LIBDIR     += ../Tuvok/Build ../Tuvok/IO/expressions
LIBS              = -lTuvok -ltuvokexpr
unix:LIBS        += -lz
win32:LIBS       += shlwapi.lib
macx-clang:QMAKE_CXXFLAGS += -stdlib=libc++
macx-clang:QMAKE_CFLAGS +=
macx-clang:LIBS        += -stdlib=libc++ -framework CoreFoundation
QMAKE_CXXFLAGS_WARN_ON += -Wno-unknown-pragmas
!macx-clang:unix:QMAKE_LFLAGS += -fopenmp

# Try to link to GLU statically, sometimes the shared lib isn't there.
gludirs = /usr/lib /usr/lib/x86_64-linux-gnu
found = false
for(d, gludirs) {
  if (static) {
      if(exists($${d}/libGLU.a)) {
        LIBS += $${d}/libGLU.a
        found = true
      }
  }
}
if(!found) { unix:!macx-clang:LIBS += -lGLU }

# Find the location of QtGui's prl file, and include it here so we can look at
# the QMAKE_PRL_CONFIG variable.
TEMP = $$[QT_INSTALL_LIBS] libQtGui.prl
PRL  = $$[QT_INSTALL_LIBS] QtGui.framework/QtGui.prl
TEMP = $$join(TEMP, "/")
PRL  = $$join(PRL, "/")
exists($$TEMP) {
  include($$TEMP)
}
exists($$PRL) {
  include($$PRL)
}

### Should we link Qt statically or as a shared lib?
# If the PRL config contains the `shared' configuration, then the installed
# Qt is shared.  In that case, disable the image plugins.
contains(QMAKE_PRL_CONFIG, shared) {
  QTPLUGIN -= qgif qjpeg qtiff
} else {
  QTPLUGIN += qgif qjpeg qtiff
}

RESOURCES         = ImageVis3D.qrc
RC_FILE 	  = Resources/ImageVis3D.rc
QMAKE_INFO_PLIST  = ../IV3D.plist
ICON              = Resources/ImageVis3D.icns
unix:QMAKE_CXXFLAGS += -std=c++0x
unix:QMAKE_CXXFLAGS += -fno-strict-aliasing
unix:QMAKE_CFLAGS += -fno-strict-aliasing

# Find the location of QtGui's prl file, and include it here so we can look at
# the QMAKE_PRL_CONFIG variable.
TEMP = $$[QT_INSTALL_LIBS] libQtGui.prl
PRL  = $$[QT_INSTALL_LIBS] QtGui.framework/QtGui.prl
TEMP = $$join(TEMP, "/")
PRL  = $$join(PRL, "/")
exists($$TEMP) {
  include($$TEMP)
}
exists($$PRL) {
  include($$PRL)
}

### Should we link Qt statically or as a shared lib?
# If the PRL config contains the `shared' configuration, then the installed
# Qt is shared.  In that case, disable the image plugins.
contains(QMAKE_PRL_CONFIG, shared) {
  message("Shared build, ensuring there will be image plugins linked in.")
  QTPLUGIN -= qgif qjpeg qtiff
} else {
  message("Static build, forcing image plugins to get loaded.")
  QTPLUGIN += qgif qjpeg qtiff
}

# Input
HEADERS += StdDefines.h \
           UI/SettingsDlg.h \
           UI/BasicSettingsDlg.h \
           UI/BrowseData.h \
           UI/ImageVis3D.h \
           UI/PleaseWait.h \
           UI/FTPDialog.h \
           UI/QTransferFunction.h \
           UI/Q1DTransferFunction.h \
           UI/Q2DTransferFunction.h \
           UI/QDataRadioButton.h \
           UI/QLightPreview.h \
           UI/RenderWindow.h \
           UI/RenderWindowGL.h \
           UI/RAWDialog.h \
           UI/MIPRotDialog.h \ 
           UI/Welcome.h \
           UI/MetadataDlg.h \
           UI/AboutDlg.h \
           UI/URLDlg.h \
           UI/BugRepDlg.h \
           UI/LODDlg.h \
           UI/MDIRenderWin.h \
           UI/MergeDlg.h \
           UI/CrashDetDlg.h \
           UI/ScaleAndBiasDlg.h \
           DebugOut/QTOut.h \
           DebugOut/QTLabelOut.h \
           IO/DialogConverter.h \
           IO/ZipFile.h \
           IO/3rdParty/crypt.h \
           IO/3rdParty/ioapi.h \
           IO/3rdParty/zip.h \
           IO/3rdParty/QFtp/qftp.h \
           IO/3rdParty/QFtp/qurlinfo.h \
           UI/DebugScriptWindow.h

FORMS += UI/UI/BrowseData.ui \
         UI/UI/ImageVis3D.ui \
         UI/UI/PleaseWait.ui \
         UI/UI/SettingsDlg.ui \
         UI/UI/RAWDialog.ui \
         UI/UI/FTPDialog.ui \
         UI/UI/Welcome.ui \
         UI/UI/Metadata.ui \
         UI/UI/CrashDetDlg.ui \
         UI/UI/About.ui \
         UI/UI/URLDlg.ui \
         UI/UI/LODDlg.ui \
         UI/UI/BugRepDlg.ui \
         UI/UI/MIPRotDialog.ui \
         UI/UI/ScaleAndBiasDlg.ui \
         UI/UI/MergeDlg.ui

SOURCES += UI/BrowseData.cpp \
           UI/ImageVis3D.cpp \
           UI/ImageVis3D_Capturing.cpp \
           UI/ImageVis3D_Progress.cpp \
           UI/ImageVis3D_1DTransferFunction.cpp \
           UI/ImageVis3D_2DTransferFunction.cpp \
           UI/ImageVis3D_FileHandling.cpp \
           UI/ImageVis3D_WindowHandling.cpp \
           UI/ImageVis3D_DebugWindow.cpp \
           UI/ImageVis3D_Settings.cpp \
           UI/ImageVis3D_Locking.cpp \
           UI/ImageVis3D_Stereo.cpp \
           UI/ImageVis3D_Help.cpp \
           UI/ImageVis3D_I3M.cpp \
           UI/PleaseWait.cpp \
           UI/Welcome.cpp \
           UI/MDIRenderWin.cpp \
           UI/MetadataDlg.cpp \
           UI/AboutDlg.cpp \
           UI/URLDlg.cpp \
           UI/FTPDialog.cpp \
           UI/BugRepDlg.cpp \
           UI/LODDlg.cpp \           
           UI/QTransferFunction.cpp \
           UI/Q1DTransferFunction.cpp \
           UI/Q2DTransferFunction.cpp \
           UI/QDataRadioButton.cpp \
           UI/QLightPreview.cpp \          
           UI/RenderWindowGL.cpp \
           UI/RenderWindow.cpp \
           UI/BasicSettingsDlg.cpp \
           UI/SettingsDlg.cpp \
           UI/RAWDialog.cpp \
           UI/MIPRotDialog.cpp \           
           UI/MergeDlg.cpp \
           UI/CrashDetDlg.cpp \
           UI/ScaleAndBiasDlg.cpp \
           DebugOut/QTOut.cpp \
           DebugOut/QTLabelOut.cpp \
           IO/DialogConverter.cpp \
           IO/ZipFile.cpp \
           IO/3rdParty/ioapi.c \
           IO/3rdParty/zip.c \
           IO/3rdParty/QFtp/qftp.cpp \
           IO/3rdParty/QFtp/qurlinfo.cpp \
           main.cpp \
           UI/DebugScriptWindow.cpp

win32 {
  HEADERS += UI/RenderWindowDX.h
  SOURCES += UI/RenderWindowDX.cpp
}
