TEMPLATE          = app
CONFIG           += exceptions qt rtti staticlib static stl warn_on
TARGET            = BatchRenderer
DEPENDPATH        = .
INCLUDEPATH       = . ../Tuvok ../Tuvok/3rdParty ../Tuvok/Basics/3rdParty 
macx:INCLUDEPATH += /usr/X11R6/include
macx:QMAKE_LIBDIR+= /usr/X11R6/lib
QMAKE_LIBDIR     += ../../Build ../../IO/expressions
QT               += opengl
LIBS             += -lTuvok -ltuvokexpr -lz
unix:LIBS        += -lGL -lX11
unix:!macx:LIBS  += -lGLU
# Try to link to GLU statically.
gludirs = /usr/lib /usr/lib/x86_64-linux-gnu
for(d, gludirs) {
  if(exists($${d}/libGLU.a) && static) {
    LIBS -= -lGLU;
    LIBS += $${d}/libGLU.a
  }
}
unix:QMAKE_CXXFLAGS += -std=c++0x
unix:QMAKE_CXXFLAGS += -fno-strict-aliasing -g
unix:QMAKE_CFLAGS += -fno-strict-aliasing -g
macx:QMAKE_CXXFLAGS += -stdlib=libc++ -mmacosx-version-min=10.7
macx:QMAKE_CFLAGS += -mmacosx-version-min=10.7
macx:LIBS        += -stdlib=libc++ -framework CoreFoundation -framework OpenGL -mmacosx-version-min=10.7

### Should we link Qt statically or as a shared lib?
# Find the location of QtCore's prl file, and include it here so we can look at
# the QMAKE_PRL_CONFIG variable.
TEMP = $$[QT_INSTALL_LIBS] libQtCore.prl
PRL  = $$[QT_INSTALL_LIBS] QtCore.framework/QtCore.prl
TEMP = $$join(TEMP, "/")
PRL  = $$join(PRL, "/")
exists($$TEMP) {
  include($$join(TEMP, "/"))
}
exists($$PRL) {
  include($$join(PRL, "/"))
}

# If that contains the `shared' configuration, the installed Qt is shared.
# In that case, disable the image plugins.
contains(QMAKE_PRL_CONFIG, shared) {
  QTPLUGIN -= qgif qjpeg
} else {
  QTPLUGIN += qgif qjpeg
}

SOURCES += \
  main.cpp \
  batchContext.cpp

unix:!macx { SOURCES += glx-context.cpp }
macx { SOURCES += cgl-context.cpp agl-context.cpp }
win32 { SOURCES += wgl-context.cpp }

HEADERS += \
  batchContext.h \
  cgl-context.h \
  glx-context.h \
  wgl-context.h
