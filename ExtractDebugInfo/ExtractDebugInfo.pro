TEMPLATE          = app
win32:TEMPLATE    = vcapp
CONFIG           += exceptions largefile rtti static stl warn_on
CONFIG           -= app_bundle
macx:DEFINES     += QT_MAC_USE_COCOA=1
TARGET            = Build/ExtractDebugInfo
macx {
  DESTDIR         = Build
  TARGET          = ExtractDebugInfo
}
DEPENDPATH       += .
INCLUDEPATH      += . ../ ../Tuvok/Basics/3rdParty ../Tuvok
QMAKE_LIBDIR     += ../Tuvok/Build ../Tuvok/IO/expressions
LIBS              = -lTuvok -ltuvokexpr
QT               += opengl
unix:LIBS        += -lz
win32:LIBS       += shlwapi.lib
unix:!macx:LIBS  += -lGLU
QMAKE_CXXFLAGS_WARN_ON += -Wno-unknown-pragmas
!macx:unix:QMAKE_LFLAGS += -fopenmp

# Try to link to GLU statically.
gludirs = /usr/lib /usr/lib/x86_64-linux-gnu
for(d, gludirs) {
  if (static) {
    if(exists($${d}/libGLU.a)) {
      LIBS -= -lGLU
      LIBS += $${d}/libGLU.a
    }
  }
}
unix:QMAKE_CXXFLAGS += -std=c++0x
unix:QMAKE_CXXFLAGS += -fno-strict-aliasing
unix:QMAKE_CFLAGS += -fno-strict-aliasing

macx:QMAKE_CXXFLAGS += -stdlib=libc++ 
macx:QMAKE_CFLAGS += 
macx:LIBS        += -stdlib=libc++ -framework CoreFoundation 

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

# Input

SOURCES += main.cpp
