#!/bin/bash
# Script to build everything we can in a single invocation, using
# a set of options which is appropriate for creating release builds.

IV3D_BUILD_TYPE="release"
VIS="-fvisibility=hidden"
INL="-fvisibility-inlines-hidden"
COVERAGE=""
CF="-Wall -Wextra -O2 -fstack-protector ${COVERAGE}"
CXF="-D_GLIBCXX_CONCEPT_CHECK -fstack-protector -DQT_NODEBUG ${COVERAGE}"
QLF=""
MKSPEC=""

if test "$1" == "32" ; then
  CF="$CF -m32"
  COVERAGE="$COVERAGE -m32"
fi

LDFLAGS="${COVERAGE}"

# Users can set the QT_BIN env var to point at a different Qt implementation.
if test -n "${QT_BIN}" ; then
  echo "Using qmake from '${QT_BIN}' instead of the default from PATH."
  qm="${QT_BIN}/qmake"
else
  qm="qmake"
fi

dirs="."
if test `uname` != "Darwin" ; then
  dirs="$dirs Tuvok/IO/test"
  CXF="${CXF} -Werror --param ssp-buffer-size=4"
  CF="${CF} --param ssp-buffer-size=4"
  QLF="${QLF}"
else
  # We don't turn -Werror on because of warnings that deal 
  # with generated code, and some unused template specialization
  # warnings. 
  MKSPEC="-spec unsupported/macx-clang"
fi
echo "Configuring..."
for d in $dirs ; do
  pushd ${d} &> /dev/null || exit 1
    ${qm} \
      -makefile \
      ${MKSPEC} \
      CONFIG+="release" \
      QMAKE_CFLAGS+="${VIS} ${CF}" \
      QMAKE_CXXFLAGS+="${VIS} ${INL} ${CF} ${CXF}" \
      QMAKE_LFLAGS+="${VIS} ${COVERAGE} ${QLF}" \
      -recursive || exit 1
  popd &> /dev/null
done

# Unless the user gave us input as to options to use, default to a small-scale
# parallel build.
if test -z "${MAKE_OPTIONS}" ; then
  MAKE_OPTIONS="-j2 -l 2.0"
fi

echo "BUILDING Tuvok..."
make --no-print-directory ${MAKE_OPTIONS} || exit 1

# Darwin's compiler is broken w.r.t. tr1, our IO tests won't work.
if test `uname -s` != "Darwin" ; then
  pushd Tuvok/IO/test &> /dev/null || exit 1
    make --no-print-directory ${MAKE_OPTIONS} || exit 1
  popd &> /dev/null
fi

echo "Bundling..."
if test `uname -s` = "Darwin" ; then
  bash Scripts/mk_app.sh $IV3D_BUILD_TYPE
else
  bash Scripts/mk_tarball.sh $IV3D_BUILD_TYPE
fi
