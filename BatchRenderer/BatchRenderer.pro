TEMPLATE            = app
CONFIG             += c++11 exceptions qt rtti staticlib static stl warn_on
linux* { CONFIG += x11 }
QT                 += gui opengl
DESTDIR             = Build
TARGET              = BatchRenderer
DEPENDPATH          = .
INCLUDEPATH         = . ../Tuvok
INCLUDEPATH        += ../Tuvok/3rdParty
INCLUDEPATH        += ../Tuvok/Basics/3rdParty
INCLUDEPATH        += ../Tuvok/3rdParty/GLEW
INCLUDEPATH        += ../Tuvok/IO/3rdParty/boost/
QMAKE_LIBDIR       += ../Tuvok/Build ../Tuvok/IO/expressions
LIBS               += -lTuvok -ltuvokexpr
linux*:LIBS        += -lGL -lz

include(../Tuvok/flags.pro)

### Should we link Qt statically or as a shared lib?
# Find the location of QtCore`s prl file, and include it here so we can look at
# the QMAKE_PRL_CONFIG variable.
#TEMP = $$[QT_INSTALL_LIBS] libQtCore.prl
#PRL  = $$[QT_INSTALL_LIBS] QtCore.framework/QtCore.prl
#TEMP = $$join(TEMP, "/")
#PRL  = $$join(PRL, "/")
#exists($$TEMP) {
#  include($$join(TEMP, "/"))
#}
#exists($$PRL) {
#  include($$join(PRL, "/"))
#}

#QTPLUGIN.platforms = qminimal
#QTPLUGIN.platforms =-
# If that contains the `shared` configuration, the installed Qt is shared.
# In that case, disable the image plugins.
#contains(QMAKE_PRL_CONFIG, shared) {
#  QTPLUGIN -= qgif qjpeg
#} else {
#  QTPLUGIN += qgif qjpeg
#}

SOURCES += \
  main.cpp \
  BatchContext.cpp \
  TuvokLuaScriptExec.cpp


linux* { SOURCES += GLXContext.cpp }
macx   { SOURCES += CGLContext.cpp }
macx   { OBJECTIVE_SOURCES += NSContext.mm }
win32  { SOURCES += WGLContext.cpp }

HEADERS += \
  BatchContext.h \
  CGLContext.h \
  NSContext.h \
  GLXContext.h \
  WGLContext.h \
  TuvokLuaScriptExec.h
