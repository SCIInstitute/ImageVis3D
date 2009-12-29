configure ^
	-debug-and-release ^
	-opensource ^
	-static ^
	-qt-libpng ^
	-qt-libtiff ^
	-qt-libjpeg ^
	-no-sql-sqlite ^
	-no-sql-sqlite2 ^
	-no-xmlpatterns ^
	-no-phonon ^
	-no-phonon-backend ^
	-no-scripttools ^
	-no-dbus ^
	-no-qt3support ^
	-no-webkit ^
	-make libs ^
	-make tools
nmake
nmake install