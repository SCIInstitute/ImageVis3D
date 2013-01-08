TEMPLATE          = app
win32:TEMPLATE    = vcapp
CONFIG           += exceptions largefile rtti static stl warn_on
macx:CONFIG      += app_bundle
macx:DEFINES     += QT_MAC_USE_COCOA=1
TARGET            = Build/UVFConverter
unix:TARGET       = Build/uvfconvert
QT               += opengl
incpath           = . ../Tuvok/IO/3rdParty/boost ../Tuvok/3rdParty/GLEW
incpath          += ../Tuvok/IO/exception
incpath          += ../Tuvok/Basics
incpath          += ../Tuvok/Basics/3rdParty ../Tuvok
DEPENDPATH       += $$incpath
INCLUDEPATH      += $$incpath
QMAKE_LIBDIR     += ../Tuvok/Build ../Tuvok/IO/expressions
LIBS              = -lTuvok -ltuvokexpr
unix:LIBS        += -lz
win3:LIBS       += shlwapi.lib
unix:!macx:LIBS  += -lGLU -lGL
macx:QMAKE_CXXFLAGS += -stdlib=libc++ -mmacosx-version-min=10.7
macx:QMAKE_CFLAGS += -mmacosx-version-min=10.7
macx:LIBS        += -stdlib=libc++ -framework CoreFoundation -mmacosx-version-min=10.7
QMAKE_CXXFLAGS_WARN_ON += -Wno-unknown-pragmas
# Try to link to GLU statically.
gludirs = /usr/lib /usr/lib/x86_64-linux-gnu
for(d, gludirs) {
  if(exists($${d}/libGLU.a) && static) {
    LIBS -= -lGLU;
    LIBS += $${d}/libGLU.a
  }
}

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
# Try to link to GLU statically.
gludirs = /usr/lib /usr/lib/x86_64-linux-gnu
for(d, gludirs) {
  if(exists($${d}/libGLU.a) && static) { LIBS -= -lGLU; LIBS += $${d}/libGLU.a }
}
unix:QMAKE_CXXFLAGS += -std=c++0x
unix:QMAKE_CXXFLAGS += -fno-strict-aliasing
unix:QMAKE_CFLAGS += -fno-strict-aliasing

# Input
HEADERS += DebugOut/HRConsoleOut.h


SOURCES += DebugOut/HRConsoleOut.cpp \
           main.cpp
