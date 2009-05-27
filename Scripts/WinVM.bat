cd..
set path=%path%;C:\Program Files (x86)\CollabNet Subversion Client;C:\Program Files (x86)\CollabNet Subversion;C:\Program Files (x86)\Inno Setup 5;C:\tools

SETLOCAL ENABLEDELAYEDEXPANSION 

date /t > result.txt
time /t  >> result.txt
echo Start >> result.txt

svn cleanup
cd Tuvok
  svn cleanup
  cd Basics
    svn cleanup
  cd ..
cd ..
svn update

time /t  >> result.txt
echo SVN completed >> result.txt

svn info > rev1.txt
cd Tuvok
svn info > ..\rev2.txt
cd ..
pushd Scripts\installer
  del /f ImageVis3D.pdf
  wget --no-check-certificate https://gforge.sci.utah.edu/gf/download/docmanfileversion/8/140/ImageVis3D.pdf
popd


for /f "tokens=1,2" %%i in (rev1.txt) do if %%i==Revision: set IV3DVERSION=%%j
for /f "tokens=1,2" %%i in (rev2.txt) do if %%i==Revision: set TUVOKVERSION=%%j
for /f "tokens=2,3" %%i in (ImageVis3D\StdDefines.h) do if %%i==IV3D_VERSION set IV3DCODEVERSION=%%j
for /f "tokens=2,3" %%i in (Tuvok\StdTuvokDefines.h) do if %%i==TUVOK_VERSION set TUVOCCODEVERSION=%%j

del rev1.txt
del rev2.txt

echo #ifndef IV3D_SVN_VERSION >> ImageVis3D\StdDefines.h
echo #define IV3D_SVN_VERSION %IV3DVERSION% >> ImageVis3D\StdDefines.h
echo #endif >> ImageVis3D\StdDefines.h

echo #ifndef TUVOK_SVN_VERSION >> Tuvok\StdTuvokDefines.h
echo #define TUVOK_SVN_VERSION %TUVOKVERSION% >> Tuvok\StdTuvokDefines.h
echo #endif >> Tuvok\StdTuvokDefines.h


set REVSTR=%IV3DVERSION%_%TUVOKVERSION%
set CONFIG=Release (with DirectX)
set QTDIR32=C:\QT\4.5.1-32bit-static\
set QTDIR64=C:\QT\4.5.1-64bit-static\

echo f | xcopy result.txt \\geronimo\share\IV3D-WIN\ImageVis3D_%IV3DCODEVERSION%_Win_r%REVSTR%.log /Y

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
rem devenv ImageVis3D.sln /rebuild "%CONFIG%|x64" /out out64.txt
msbuild ImageVis3D.sln /nologo /p:Configuration="%CONFIG%",Platform=x64 /t:rebuild /fileLogger  /fileLoggerParameters:LogFile=out64.txt 
IF ERRORLEVEL 0 (
  set FAILED64=FALSE
) ELSE (
  set FAILED64=TRUE
)

time /t  >> result.txt
IF EXIST "Build\x64\%CONFIG%\ImageVis3D-64.exe" (
  set BUILD64=TRUE
  echo ImageVis 64bit build completed >> result.txt
  iscc Scripts\installer\64.iss
) ELSE (
  set BUILD64=FALSE
  echo ImageVis 64bit build failed >> result.txt
)

IF EXIST "CmdLineConverter\Build\x64\%CONFIG%\UVFConverter64.exe" (
  set BUILDUVF64=TRUE
  echo UVFConverter64 completed >> result.txt
) ELSE (
  set BUILDUVF64=FALSE
  echo UVFConverter64 failed >> result.txt
)

IF EXIST out32.txt del out32.txt
rem devenv ImageVis3D.sln /rebuild "%CONFIG%|win32" /out out32.txt
msbuild ImageVis3D.sln /nologo /p:Configuration="%CONFIG%",Platform=win32 /t:rebuild /fileLogger /fileLoggerParameters:LogFile=out32.txt 
IF ERRORLEVEL 0 (
  set FAILED32=FALSE
) ELSE (
  set FAILED32=TRUE
)

time /t  >> result.txt
IF EXIST "Build\Win32\%CONFIG%\ImageVis3D-32.exe" (
  set BUILD32=TRUE
  echo ImageVis 32bit build completed >> result.txt
  iscc Scripts\installer\32.iss
) ELSE (
  set BUILD32=FALSE
  echo ImageVis 32bit build failed >> result.txt
)

IF EXIST "CmdLineConverter\Build\Win32\%CONFIG%\UVFConverter32.exe" (
  set BUILDUVF32=TRUE
  echo UVFConverter32 completed >> result.txt
) ELSE (
  set BUILDUVF32=FALSE
  echo UVFConverter32 failed >> result.txt
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

IF EXIST ..\ImageVis3D_%IV3DCODEVERSION%_Win_r%REVSTR%.zip (
  time /t  >> ..\result.txt
  echo ImageVis3D build successful >> ..\result.txt
) else (
  goto ZIPFAIL
)

echo %IV3DCODEVERSION% >   Windows_Latest_Version.txt
echo %IV3DVERSION% >>      Windows_Latest_Version.txt
echo %TUVOCCODEVERSION% >> Windows_Latest_Version.txt
echo %TUVOKVERSION% >>     Windows_Latest_Version.txt
xcopy Windows_Latest_Version.txt \\geronimo\share\IV3D-WIN /Y
del Windows_Latest_Version.txt


if NOT !BUILDUVF64!==TRUE (
  if NOT !BUILDUVF32!==TRUE goto UVFAILED
)

if !BUILDUVF64!==TRUE xcopy "..\CmdLineConverter\Build\x64\%CONFIG%\UVFConverter64.exe" .
if !BUILDUVF32!==TRUE xcopy "..\CmdLineConverter\Build\Win32\%CONFIG%\UVFConverter32.exe" .

"C:\Program Files\7-Zip\7z" a -r ..\UVFConverter_Win_r%REVSTR%.zip
del . /F /S /Q

xcopy ..\ImageVis3D_%IV3DCODEVERSION%_Win_r%REVSTR%.zip \\geronimo\share\IV3D-WIN /Y
del ..\ImageVis3D_%IV3DCODEVERSION%_Win_r%REVSTR%.zip

if !BUILD32!==TRUE (
  echo f | xcopy ..\Scripts\installer\ImageVis3D-1.0-32bit.exe \\geronimo\share\IV3D-WIN\ImageVis3D_%IV3DCODEVERSION%_Win_r%REVSTR%-32bit-installer.exe /Y
  echo f | xcopy ..\Scripts\installer\ImageVis3D-1.0-64bit.exe \\geronimo\share\IV3D-WIN\ImageVis3D-Latest-32bit-installer.exe /Y
)

if !BUILD64!==TRUE (
  echo f | xcopy ..\Scripts\installer\ImageVis3D-1.0-64bit.exe \\geronimo\share\IV3D-WIN\ImageVis3D_%IV3DCODEVERSION%_Win_r%REVSTR%-64bit-installer.exe /Y
  echo f | xcopy ..\Scripts\installer\ImageVis3D-1.0-64bit.exe \\geronimo\share\IV3D-WIN\ImageVis3D-Latest-64bit-installer.exe /Y
)


IF EXIST ..\UVFConverter_Win_r%REVSTR%.zip (
  time /t  >> ..\result.txt
  echo UVFConverter build successful >> ..\result.txt
) else (
  goto ZIPFAILUVF
)

xcopy ..\UVFConverter_Win_r%REVSTR%.zip \\geronimo\share\IV3D-WIN /Y
del ..\UVFConverter_Win_r%REVSTR%.zip

:UVFAILED

cd ..
rmdir Nightly

echo f | xcopy "Tuvok\Build\Win32\%CONFIG%\objects\BuildLog.htm" \\geronimo\share\IV3D-WIN\tuvok32.htm /Y
echo f | xcopy "Tuvok\Build\x64\%CONFIG%\objects\BuildLog.htm" \\geronimo\share\IV3D-WIN\tuvok64.htm /Y
echo f | xcopy "Build\Win32\%CONFIG%\objects\BuildLog.htm" \\geronimo\share\IV3D-WIN\IV3D32.htm /Y
echo f | xcopy "Build\x64\%CONFIG%\objects\BuildLog.htm" \\geronimo\share\IV3D-WIN\IV3D64.htm /Y

GOTO END

:ENVFAIL

echo Could not set Visual Studio Environment >> result.txt
goto END

:ZIPFAIL

echo Packing the final ImageVis3D ZIP file failed  >> result.txt
goto END

:ZIPFAILUVF

echo Packing the final UVFConverter ZIP file failed  >> result.txt
goto END

:ALLFAILED

echo All builds failed to compile >> result.txt
goto END

:END

echo.>> result.txt
echo.>> result.txt
echo -------------------------------->> result.txt
echo.>> result.txt
echo.>> result.txt
type out32.txt >> result.txt
type out64.txt >> result.txt

echo f | xcopy result.txt \\geronimo\share\IV3D-WIN\ImageVis3D_%IV3DCODEVERSION%_Win_r%REVSTR%.log /Y

IF EXIST result.txt del result.txt
IF EXIST out32.txt del out32.txt
IF EXIST out64.txt del out64.txt

del ImageVis3D\StdDefines.h
del Tuvok\StdTuvokDefines.h

echo done > \\geronimo\share\IV3D-WIN\done.trigger