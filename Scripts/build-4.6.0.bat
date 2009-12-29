set VER=4.6.0
rm -fr qt-%VER%
"C:/program files/7-zip/7z.exe" x qt-everywhere-opensource-src-%VER%.zip
mv qt-everywhere-opensource-src-%VER% qt-%VER%-64bit
cd qt-%VER%-64bit
cp ../Win-StaticQt-%VER%.bat ./
Win-StaticQt-%VER%.bat
