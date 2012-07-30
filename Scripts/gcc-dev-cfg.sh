#!/bin/bash

#-D_REENTRANT -DQT_NO_DEBUG -DQT_GUI_LIB -DQT_CORE_LIB -DQT_SHARED
VIS="-fvisibility=hidden"
INL="-fvisibility-inlines-hidden"
CF="-Wall -Wextra -O0 -D_DEBUG"
CXF="-D_GLIBCXX_CONCEPT_CHECK"
LF=""
MKSPEC=""
if test `uname -s` != "Darwin" ; then
  CF="${CF} -ggdb3 "
  CXF="${CXF} -D_GLIBCXX_DEBUG -Werror"
else
  CXF="${CXF} -D_GLIBCXX_DEBUG -std=c++0x -stdlib=libc++"
  MKSPEC="-spec unsupported/macx-clang"
  LF="QMAKE_LFLAGS=\"-stdlib=libc++\""
fi
if test -n "${QT_BIN}" ; then
    echo "Using custom qmake..."
    qm="${QT_BIN}/qmake"
else
    qm="qmake"
fi
for d in . ; do
  pushd ${d} &>/dev/null
    ${qm} \
        ${MKSPEC} \
        ${LF} \
        QMAKE_CONFIG+="debug" \
        CONFIG+="debug" \
        QMAKE_CFLAGS="${VIS} ${CF}" \
        QMAKE_CXXFLAGS="${VIS} ${INL} ${CF} ${CXF}" \
        QMAKE_CFLAGS_RELEASE="-O0" \
        QMAKE_CXXFLAGS_RELEASE="-O0" \
        -recursive || exit 1
  popd &>/dev/null
done
