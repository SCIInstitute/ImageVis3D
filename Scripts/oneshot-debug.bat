; Script to build everything we can in a single invocation, using
; a set of options which is appropriate for creating debug builds.

set w32_cf="-D_CRT_SECURE_NO_WARNINGS=1 -D_SCL_SECURE_NO_WARNINGS=1"
qmake -tp vc ^
  QMAKE_CFLAGS+=%w32_cf% ^
  QMAKE_CXXFLAGS+=%w32_cf% ^
  QMAKE_LFLAGS+=%w32_cf% ^
  -recursive ^
  ImageVis3d.pro
; hardcoding vs2008 for now =(
bld="C:\Program Files (x86)\Microsoft Visual Studio 9.0\Common7\IDE\devenv.COM"
%bld% ^
  ImageVis3D.sln ^
  /nologo ^
  /Rebuild

pushd Tuvok\IO\test
  python ../3rdParty/cxxtest/cxxtestgen.py ^
    --no-static-init ^
    --error-printer ^
    -o alltests.cpp ^
    quantize.h ^
    jpeg.h

  qmake -tp vc ^
    QMAKE_CFLAGS+=%w32_cf% ^
    QMAKE_CXXFLAGS+=%w32_cf% ^
    QMAKE_LFLAGS+=%w32_cf% ^
    -recursive ^
    test.pro

  %bld% ^
    test.sln ^
    /nologo ^
    /Rebuild
popd

; bundle it.
iscc Scripts/installer/64.iss
