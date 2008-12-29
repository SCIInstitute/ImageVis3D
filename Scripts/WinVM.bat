cd..
set path=%path%;C:\Program Files (x86)\CollabNet Subversion

SETLOCAL ENABLEDELAYEDEXPANSION 

date /t > result.txt
time /t  >> result.txt
echo Start >> result.txt

svn up

time /t  >> result.txt
echo SVN completed >> result.txt

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

IF EXIST "C:\Program Files (x86)\Microsoft Visual Studio 9.0\VC\vcvarsall.bat" (
  call "C:\Program Files (x86)\Microsoft Visual Studio 9.0\VC\vcvarsall.bat" amd64
  goto ENVSET
)

IF EXIST "C:\Program Files\Microsoft Visual Studio 9.0\VC\vcvarsall.bat" (
  call "C:\Program Files\Microsoft Visual Studio 9.0\VC\vcvarsall.bat" x86
  goto ENVSET
)

IF EXIST "C:\Program Files (x86)\Microsoft Visual Studio 8.0\VC\vcvarsall.bat" (
  call "C:\Program Files (x86)\Microsoft Visual Studio 8.0\VC\vcvarsall.bat" amd64
  goto ENVSET
)

IF EXIST "C:\Program Files\Microsoft Visual Studio 8.0\VC\vcvarsall.bat" (
  call "C:\Program Files\Microsoft Visual Studio 8.0\VC\vcvarsall.bat" x86
  goto ENVSET
)


goto ENVFAIL

:ENVSET

time /t  >> result.txt
echo Build Environment completed >> result.txt

echo on

IF EXIST out64.txt del out64.txt
devenv ImageVis3D.sln /rebuild "%CONFIG%|x64" /out out64.txt
IF ERRORLEVEL 0 (
  set FAILED64=FALSE
) ELSE (
  set FAILED64=TRUE
)

time /t  >> result.txt
IF EXIST "Build\x64\%CONFIG%\ImageVis3D-64.exe" (
  set BUILD64=TRUE
  echo 64bit build completed >> result.txt
) ELSE (
  set BUILD64=FALSE
  echo 64bit build failed >> result.txt
)

IF EXIST out32.txt del out32.txt
devenv ImageVis3D.sln /rebuild "%CONFIG%|win32" /out out32.txt
IF ERRORLEVEL 0 (
  set FAILED32=FALSE
) ELSE (
  set FAILED32=TRUE
)

time /t  >> result.txt
IF EXIST "Build\Win32\%CONFIG%\ImageVis3D-32.exe" (
  set BUILD32=TRUE
  echo 32bit build completed >> result.txt
) ELSE (
  set BUILD32=FALSE
  echo 32bit build failed >> result.txt
)

IF NOT EXIST \\geronimo\share\IV3D-WIN\nul mkdir \\geronimo\share\IV3D-WIN

if NOT !BUILD64!==TRUE (
  if NOT !BUILD32!==TRUE goto ALLFAILED
)

time /t  >> result.txt
echo Packing ZIP file >> result.txt

mkdir Nightly
cd Nightly
if !BUILD64!==TRUE xcopy "..\Build\x64\%CONFIG%\ImageVis3D-64.exe" .
if !BUILD32!==TRUE xcopy "..\Build\Win32\%CONFIG%\ImageVis3D-32.exe" .
mkdir Shaders
xcopy ..\Tuvok\Shaders\*.glsl .\Shaders

del ..\ImageVis3D*.zip
"C:\Program Files\7-Zip\7z" a -r ..\ImageVis3D_%IV3DCODEVERSION%_Win_r%REVSTR%.zip

del . /F /S /Q
rmDir Shaders
cd ..
rmdir Nightly

IF EXIST ImageVis3D_%IV3DCODEVERSION%_Win_r%REVSTR%.zip (
  time /t  >> result.txt
  echo Nightly build successful >> result.txt
  echo.>> result.txt
  echo.>> result.txt
  echo -------------------------------->> result.txt
  echo.>> result.txt
  echo.>> result.txt
  type out32.txt >> result.txt
  type out64.txt >> result.txt
) else (
  goto ZIPFAIL
)

xcopy ImageVis3D*.zip \\geronimo\share\IV3D-WIN /Y
del ImageVis3D*.zip

copy "Tuvok\Build\Win32\%CONFIG%\objects\BuildLog.htm" \\geronimo\share\IV3D-WIN\tuvok32.htm /Y
copy "Tuvok\Build\x64\%CONFIG%\objects\BuildLog.htm" \\geronimo\share\IV3D-WIN\tuvok64.htm /Y
copy "Build\Win32\%CONFIG%\objects\BuildLog.htm" \\geronimo\share\IV3D-WIN\IV3D32.htm /Y
copy "Build\x64\%CONFIG%\objects\BuildLog.htm" \\geronimo\share\IV3D-WIN\IV3D64.htm /Y

GOTO END

:ENVFAIL

echo Could not set Visual Studio Environment >> result.txt
goto END

:ZIPFAIL

echo Packing the final ZIP file failed  >> result.txt
goto END

:ALLFAILED

echo All builds failed to compile >> result.txt
echo.>> result.txt
echo.>> result.txt
echo -------------------------------->> result.txt
echo.>> result.txt
echo.>> result.txt
type out32.txt >> result.txt
type out64.txt >> result.txt
goto END

:END

copy result.txt \\geronimo\share\IV3D-WIN\ImageVis3D_%IV3DCODEVERSION%_Win_r%REVSTR%.log /Y
IF EXIST out32.txt del out32.txt
IF EXIST out64.txt del out64.txt

