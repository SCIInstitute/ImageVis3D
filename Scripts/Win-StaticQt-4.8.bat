nmake clean 
configure ^
  -buildkey "imagevis3d" ^
  -debug-and-release ^
  -opensource ^
  -confirm-license ^
  -static ^
  -ltcg ^
  -exceptions ^
  -stl ^
  -no-sql-sqlite ^
  -no-sql-sqlite2 ^
  -no-qt3support ^
  -platform win32-msvc2015 ^
  -largefile ^
  -no-gif ^
  -qt-libpng ^
  -no-libmng ^
  -qt-libtiff ^
  -qt-libjpeg ^
  -no-incredibuild-xge ^
  -no-phonon ^
  -no-phonon-backend ^
  -no-multimedia ^
  -no-audio-backend ^
  -no-webkit ^
  -no-script ^
  -no-scripttools ^
  -no-declarative ^
  -no-declarative-debug ^
  -mp ^
  -nomake examples ^
  -nomake demos ^
  -nomake docs

nmake sub-src
