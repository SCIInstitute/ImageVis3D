TEMPLATE          = app
win32:TEMPLATE    = vcapp
CONFIG           += exceptions largefile qt rtti static stl warn_on
macx:CONFIG      += app_bundle
macx:DEFINES     += QT_MAC_USE_COCOA=1
TARGET            = Build/UVFConverter
unix:TARGET       = Build/uvfconvert
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
unix:LIBS        += -lz
win32:LIBS       += shlwapi.lib
macx:QMAKE_CXXFLAGS += -stdlib=libc++
macx:QMAKE_CFLAGS   +=
macx:LIBS           += -stdlib=libc++ -framework CoreFoundation
macx:LIBS           +=
QMAKE_CXXFLAGS_WARN_ON += -Wno-unknown-pragmas
!macx:unix:QMAKE_LFLAGS += -fopenmp
# Try to link to GLU statically.
gludirs = /usr/lib /usr/lib/x86_64-linux-gnu
found=false
for(d, gludirs) {
  if(exists($${d}/libGLU.a)) {
    LIBS += $${d}/libGLU.a
    found=true
  }
}
if(!found) {
  # not mac: GLU comes in the GL framework.
  unix:!macx:LIBS += -lGLU
}
unix:!macx:LIBS += -lGL

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

unix:QMAKE_CXXFLAGS += -std=c++0x
unix:QMAKE_CXXFLAGS += -fno-strict-aliasing
unix:QMAKE_CFLAGS += -fno-strict-aliasing

# Input
HEADERS += DebugOut/HRConsoleOut.h


SOURCES += DebugOut/HRConsoleOut.cpp \
           main.cpp
