echo oy | configure ^
	-static ^
	-debug-and-release ^
	-buildkey "imagevis3d" ^
	-no-sql-sqlite ^
	-no-sql-sqlite2 ^
	-no-phonon ^
	-no-scripttools ^
	-no-webkit ^
	-no-xmlpatterns ^
	-no-multimedia ^
	-no-qt3support ^
	-nomake examples ^
	-nomake demos ^
	-nomake docs ^
	-qt-libtiff ^
	-qt-libpng ^
	-qt-libjpeg
nmake sub-src