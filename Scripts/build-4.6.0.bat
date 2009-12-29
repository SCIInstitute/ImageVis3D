set VER=4.6.0
rm -fr qt-%VER%
rmdir /S /Q qt-%VER% qt-%VER%-64bit
"C:/program files/7-zip/7z.exe" x qt-everywhere-opensource-src-%VER%.zip
move /Y qt-everywhere-opensource-src-%VER% qt-%VER%-64bit
cd qt-%VER%-64bit
cp ..\Win-StaticQt-%VER%.bat .
Win-StaticQt-%VER%.bat
