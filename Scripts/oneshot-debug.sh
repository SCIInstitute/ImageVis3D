#!/bin/sh
# Script to build everything we can in a single invocation, using
# a set of options which is appropriate for creating debug builds.

VIS="-fvisibility=hidden"
INL="-fvisibility-inlines-hidden"
COVERAGE="-fprofile-arcs -ftest-coverage"
CF="-g -Wall -Wextra -O0 -D_DEBUG ${COVERAGE}"
CXF="-D_GLIBCXX_CONCEPT_CHECK -Werror ${COVERAGE}"
LDFLAGS="${COVERAGE}"
# Darwin's debug STL support is broken.
if test `uname -s` != "Darwin"; then
  CXF="${CXF} -D_GLIBCXX_DEBUG"
fi

# Users can set the QT_BIN env var to point at a different Qt implementation.
if test -n "${QT_BIN}" ; then
  echo "Using qmake from '${QT_BIN}' instead of the default from PATH."
  qm="${QT_BIN}/qmake"
else
  qm="qmake"
fi

echo "Configuring..."
for d in . Tuvok/IO/test ; do
  pushd ${d} &> /dev/null || exit 1
    ${qm} \
      QMAKE_CONFIG+="debug" \
      QMAKE_CFLAGS+="${VIS} ${CF}" \
      QMAKE_CXXFLAGS+="${VIS} ${INL} ${CF} ${CXF}" \
      QMAKE_LFLAGS+="${VIS} ${COVERAGE}" \
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
  bash Scripts/mk_app.sh
else
  bash Scripts/mk_tarball.sh
fi
