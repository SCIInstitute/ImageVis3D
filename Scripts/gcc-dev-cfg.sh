#!/bin/sh

#-D_REENTRANT -DQT_NO_DEBUG -DQT_GUI_LIB -DQT_CORE_LIB -DQT_SHARED
VIS="-fvisibility-inlines-hidden -fvisibility=hidden"
CF="-g -Wall -Wextra -O0 -D_DEBUG"
CXF="-D_GLIBCXX_DEBUG -D_GLIBCXX_CONCEPT_CHECK ${VIS} -Werror"
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
        QMAKE_CFLAGS="${CF}" \
        QMAKE_CXXFLAGS="${CF} ${CXF}" \
        -recursive
  popd &>/dev/null
done
