
set path=%path%;C:\Program Files (x86)\CollabNet Subversion

cd..
svn up

svn info > rev1.txt
cd Tuvok
svn info > ..\rev2.txt
cd ..

for /f "tokens=1,2" %%i in (rev1.txt) do if %%i==Revision: set IV3DVERSION=%%j
for /f "tokens=1,2" %%i in (rev2.txt) do if %%i==Revision: set TUVOKVERSION=%%j
for /f "tokens=2,3" %%i in (ImageVis3D\StdDefines.h) do if %%i==IV3D_VERSION set IV3DCODEVERSION=%%j

del rev1.txt
del rev2.txt

set REVSTR=%IV3DVERSION%_%TUVOKVERSION%
set CONFIG=Release (with DirectX)
set QTDIR32=C:\QT\4.4.3-32bit-static\
set QTDIR64=C:\QT\4.4.3-64bit-static\

call "C:\Program Files (x86)\Microsoft Visual Studio 9.0\VC\vcvarsall.bat" amd64
echo on

IF EXIST out64.txt del out64.txt
IF EXIST out32.txt del out32.txt
devenv ImageVis3D.sln /rebuild "%CONFIG%|x64" /out out64.txt
IF NOT ERRORLEVEL 0 GOTO FAILED64
devenv ImageVis3D.sln /rebuild "%CONFIG%|win32" /out out32.txt
IF NOT ERRORLEVEL 0 GOTO FAILED32

IF NOT EXIST "Build\x64\%CONFIG%\ImageVis3D-64.exe" GOTO NOTFOUND64
IF NOT EXIST "Build\Win32\%CONFIG%\ImageVis3D-32.exe" GOTO NOTFOUND32

mkdir Nightly
cd Nightly
xcopy "..\Build\x64\%CONFIG%\ImageVis3D-64.exe" .
xcopy "..\Build\Win32\%CONFIG%\ImageVis3D-32.exe" .
mkdir Shaders
xcopy ..\Tuvok\Shaders\*.glsl .\Shaders

del ..\ImageVis3D*.zip
"C:\Program Files\7-Zip\7z" a -r ..\ImageVis3D%IV3DCODEVERSION%-Win-%REVSTR%.zip

del . /F /S /Q
rmDir Shaders
cd ..
rmdir Nightly

date /t  > result.txt
echo Nightly build successful  >> result.txt
type out32.txt >> result.txt
type out64.txt >> result.txt

IF NOT EXIST \\geronimo\share\IV3D-WIN\nul mkdir \\geronimo\share\IV3D-WIN
xcopy ImageVis3D*.zip \\geronimo\share\IV3D-WIN /Y
del ImageVis3D*.zip

copy "Tuvok\Build\Win32\%CONFIG%\objects\BuildLog.htm" \\geronimo\share\IV3D-WIN\tuvok32.htm /Y
copy "Tuvok\Build\x64\%CONFIG%\objects\BuildLog.htm" \\geronimo\share\IV3D-WIN\tuvok64.htm /Y
copy "Build\Win32\%CONFIG%\objects\BuildLog.htm" \\geronimo\share\IV3D-WIN\IV3D32.htm /Y
copy "Build\x64\%CONFIG%\objects\BuildLog.htm" \\geronimo\share\IV3D-WIN\IV3D64.htm /Y

GOTO END

:FAILED32

date /t  > result.txt
echo 32bit compile failed >> result.txt
type out32.txt >> result.txt
type out64.txt >> result.txt
goto END

:FAILED64

date /t  > result.txt
echo 64bit compile failed >> result.txt
type out32.txt >> result.txt
type out64.txt >> result.txt
goto END

:NOTFOUND64

date /t  > result.txt
echo 64bit bin not found >> result.txt
type out32.txt >> result.txt
type out64.txt >> result.txt
goto END

:NOTFOUND32

date /t  > result.txt
echo 32bit bin not found >> result.txt
type out32.txt >> result.txt
type out64.txt >> result.txt
goto END

:END

del out32.txt
del out64.txt

IF NOT EXIST \\geronimo\share\IV3D-WIN\nul mkdir \\geronimo\share\IV3D-WIN
xcopy result.txt \\geronimo\share\IV3D-WIN
del results.txt

