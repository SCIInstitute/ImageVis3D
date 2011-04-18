#!/bin/bash

#-D_REENTRANT -DQT_NO_DEBUG -DQT_GUI_LIB -DQT_CORE_LIB -DQT_SHARED
VIS="-fvisibility=hidden"
INL="-fvisibility-inlines-hidden"
CF="-g -Wall -Wextra -O0 -D_DEBUG"
CXF="-D_GLIBCXX_CONCEPT_CHECK -Werror"
if test `uname -s` != "Darwin" ; then
  CXF="${CXF} -D_GLIBCXX_DEBUG"
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
        QMAKE_CONFIG="debug" \
        QMAKE_CFLAGS="${VIS} ${CF}" \
        QMAKE_CXXFLAGS="${VIS} ${INL} ${CF} ${CXF}" \
        -recursive
  popd &>/dev/null
done
