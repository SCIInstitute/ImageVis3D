TEMPLATE          = app
win32:TEMPLATE    = vcapp
CONFIG           += c++11 exceptions largefile qt rtti static stl warn_on
linux*:CONFIG    += x11
macx:CONFIG      += app_bundle
macx:DEFINES     += QT_MAC_USE_COCOA=1
TARGET            = Build/UVFConverter
linux*|mac*:TARGET = Build/uvfconvert
macx {
  DESTDIR         = Build
  TARGET          = uvfconvert
}
QT                = core gui opengl
incpath           = ../Tuvok/3rdParty/GLEW
incpath          += ../Tuvok/IO/3rdParty/boost/
incpath          += ../Tuvok/IO/exception
incpath          += ../Tuvok/Basics
incpath          += ../Tuvok/Basics/3rdParty
incpath          += ../Tuvok
DEPENDPATH       += $$incpath
INCLUDEPATH      += $$incpath
QMAKE_LIBDIR     += ../Tuvok/Build ../Tuvok/IO/expressions
LIBS             += -lTuvok -ltuvokexpr
linux*:LIBS      += -lz
win32:LIBS       += shlwapi.lib
include(../Tuvok/flags.pro)

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

unix:QMAKE_CXXFLAGS += -fno-strict-aliasing -fPIC
unix:QMAKE_CFLAGS += -fno-strict-aliasing -fPIC

# Input
HEADERS += DebugOut/HRConsoleOut.h


SOURCES += DebugOut/HRConsoleOut.cpp \
           main.cpp
