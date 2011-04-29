nmake clean 
configure ^
	-static ^
	-debug-and-release ^
	-buildkey "imagevis3d" ^
        -qt-libtiff ^
	-qt-libpng ^
	-qt-libjpeg ^
        -opensource ^
        -no-qt3support ^
        -no-webkit ^
        -no-phonon ^
        -confirm-license ^
        -no-openssl
        -nomake demos 
        -nomake examples 
nmake sub-src
