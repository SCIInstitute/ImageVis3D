configure ^
	-static ^
	-debug-and-release ^
	-buildkey "imagevis3d" ^
        -no-sql-sqlite ^
        -no-sql-sqlite2 ^
        -no-xmlpatterns ^
        -no-phonon ^
        -no-phonon-backend ^
        -no-webkit ^
        -no-scripttools ^
        -no-openssl ^
        -no-qt3support ^
	-qt-libpng ^
	-qt-libtiff ^
	-qt-libjpeg
nmake
