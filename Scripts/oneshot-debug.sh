#!/bin/bash
# Script to build everything we can in a single invocation, using
# a set of options which is appropriate for creating debug builds.

IV3D_BUILD_TYPE="debug"
VIS="-fvisibility=hidden"
INL="-fvisibility-inlines-hidden"
if test `uname -s` != "Darwin"; then
  if test "${CXX}" != "clang++"; then
    COVERAGE="-fprofile-arcs -ftest-coverage"
  fi
else
  COVERAGE=""
fi
CF="-g -Wall -Wextra -O0 -D_DEBUG -fstack-protector ${COVERAGE}"
CXF="-D_GLIBCXX_CONCEPT_CHECK -fstack-protector ${COVERAGE}"
MKSPEC=""
LDFLAGS="${COVERAGE}"

if test "$1" == "32" ; then
  CF="${CF} -m32"
  CXF="${CXF} -m32"
  LDFLAGS="${LDFLAGS} -m32"
fi

# Darwin's debug STL support is broken.
# Ditto: OpenMP
if test `uname -s` != "Darwin"; then
  if test "${CXX}" == "clang++"; then
    # We are using clang on a linux system. Set Qt mkspec appropriately.
    MKSPEC="-spec linux-clang"
    CXF="${CXF} -D_GLIBCXX_DEBUG"
  else
    CXF="${CXF} -D_GLIBCXX_DEBUG --param ssp-buffer-size=4 -Werror"
    LDFLAGS="${LDFLAGS} --param ssp-buffer-size=4"
  fi
else
  # We don't turn -Werror on because of warnings that deal 
  # with generated code, and some unused template specialization
  # warnings. 
  MKSPEC="-spec macx-clang"
fi

# Users can set the QT_BIN env var to point at a different Qt implementation.
if test -n "${QT_BIN}" ; then
  echo "Using qmake from '${QT_BIN}' instead of the default from PATH."
  qm="${QT_BIN}/qmake"
else
  qm="qmake"
fi
${qm} --version || exit 1

# Unless the user gave us input as to options to use, default to a small-scale
# parallel build.
if test -z "${MAKE_OPTIONS}" ; then
  MAKE_OPTIONS="-j2 -l 2.0"
fi

if test `uname -s` == "Darwin"; then
  echo "properties:"
  macos=$(${qm} -query macos)
  macx=$(${qm} -query mac)
  qmake_tgt=$(${qm} -query QMAKE_MACOSX_DEPLOYMENT_TARGET)
  tgt=$(${qm} -query deployment_target)
  echo -e "\tmacos: ${macos}"
  echo -e "\tmacx: ${macx}"
  echo -e "\tqmake deploy target: ${qmake_tgt}"
  echo -e "\tdeploy target: ${tgt}"
fi

dirs="."
dirs="${dirs} Tuvok/IO/test"
echo "Configuring..."
for d in $dirs ; do
  pushd ${d} &> /dev/null || exit 1
    ${qm} \
      ${MKSPEC} \
      CONFIG="debug" \
      QMAKE_MACOSX_DEPLOYMENT_TARGET=10.10 \
      deployment_target=10.11 \
      -recursive || exit 1
    make --no-print-directory ${MAKE_OPTIONS} || exit 1
  popd &> /dev/null
done

echo "Bundling..."
if test `uname -s` = "Darwin" ; then
  bash Scripts/mk_app.sh $IV3D_BUILD_TYPE
else
  bash Scripts/mk_tarball.sh $IV3D_BUILD_TYPE
fi
