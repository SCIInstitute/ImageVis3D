REM Script to build everything we can in a single invocation, using
REM a set of options which is appropriate for creating debug builds.

set w32_cf="-D_CRT_SECURE_NO_WARNINGS=1 -D_SCL_SECURE_NO_WARNINGS=1"
qmake -tp vc ^
  QMAKE_CFLAGS+=%w32_cf% ^
  QMAKE_CXXFLAGS+=%w32_cf% ^
  -recursive ^
  ImageVis3d.pro
REM hardcoding vs2008 for now =(
set bld="C:\Program Files (x86)\Microsoft Visual Studio 9.0\Common7\IDE\devenv.COM"
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
    -recursive ^
    test.pro

  %bld% ^
    test.sln ^
    /nologo ^
    /Rebuild
popd

REM download documentation
set manual="http://www.sci.utah.edu/images/docs/imagevis3d.pdf"
set mdata="http://ci.sci.utah.edu:8011/devbuilds/GettingDataIntoImageVis3D.pdf"
wget --no-check-certificate -q %manual%
wget --no-check-certificate -q %mdata%

REM bundle it.
iscc Scripts/installer/32-debug.iss
