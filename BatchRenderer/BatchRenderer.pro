TEMPLATE            = app
CONFIG             += exceptions qt rtti staticlib static stl warn_on
DESTDIR             = Build
TARGET              = BatchRenderer
DEPENDPATH          = .
INCLUDEPATH         = . ../Tuvok
INCLUDEPATH        += ../Tuvok/3rdParty
INCLUDEPATH        += ../Tuvok/Basics/3rdParty
INCLUDEPATH        += ../Tuvok/3rdParty/GLEW
INCLUDEPATH        += ../Tuvok/IO/3rdParty/boost/
QMAKE_LIBDIR       += ../Tuvok/Build ../Tuvok/IO/expressions
QT                 += opengl
LIBS               += -lTuvok -ltuvokexpr -lz
!macx:unix:QMAKE_LFLAGS += -fopenmp

# Operating system definitions now in the makefile instead of
# a header file.
unix:!macx  { DEFINES += "DETECTED_OS_LINUX" }
macx        { DEFINES += "DETECTED_OS_APPLE" }
win32       { DEFINES += "DETECTED_OS_WINDOWS" }

####
# General unix configuration (including Mac OS X).
####
unix:QMAKE_CXXFLAGS += -fno-strict-aliasing -g -std=c++0x 
unix:QMAKE_CFLAGS   += -fno-strict-aliasing -g

####
# Non-OSX Unix configuration
####
# Note: Do NOT specific the GL linker flag (-lGL) on Mac!
unix:!macx:LIBS    += -lGL -lX11 -lGLU
# Try to link to GLU statically.
gludirs = /usr/lib /usr/lib/x86_64-linux-gnu
#for(d, gludirs) {
#  if(exists($${d}/libGLU.a) && static) {
#    LIBS -= -lGLU;
#    LIBS += $${d}/libGLU.a
#  }
#}

####
# Mac OS X configuration
####
macx:QMAKE_CXXFLAGS         += -stdlib=libc++
macx:QMAKE_OBJECTIVE_CFLAGS += -std=c++0x -stdlib=libc++
macx:QMAKE_CFLAGS           +=
macx:LIBS                   += -stdlib=libc++ -framework Cocoa -framework OpenGL
macx:CONFIG                 -= app_bundle
macx:INCLUDEPATH            += /usr/X11R6/include
macx:QMAKE_LIBDIR           += /usr/X11R6/lib

### Should we link Qt statically or as a shared lib?
# Find the location of QtCore`s prl file, and include it here so we can look at
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

# If that contains the `shared` configuration, the installed Qt is shared.
# In that case, disable the image plugins.
contains(QMAKE_PRL_CONFIG, shared) {
  QTPLUGIN -= qgif qjpeg
} else {
  QTPLUGIN += qgif qjpeg
}

SOURCES += \
  main.cpp \
  BatchContext.cpp \
  TuvokLuaScriptExec.cpp


unix:!macx  { SOURCES += GLXContext.cpp }
macx        { SOURCES += CGLContext.cpp }
macx        { OBJECTIVE_SOURCES += NSContext.mm }
win32       { SOURCES += WGLContext.cpp }

HEADERS += \
  BatchContext.h \
  CGLContext.h \
  NSContext.h \
  GLXContext.h \
  WGLContext.h \
  TuvokLuaScriptExec.h
