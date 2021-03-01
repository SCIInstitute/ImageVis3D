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
  wget --no-check-certificate http://www.sci.utah.edu/images/docs/imagevis3d.pdf
  wget --no-check-certificate http://ci.sci.utah.edu:8011/devbuilds/GettingDataIntoImageVis3D.pdf
  rename imagevis3d.pdf ImageVis3D.pdf
popd


for /f "tokens=1,2" %%i in (rev1.txt) do if %%i==Revision: set IV3DVERSION=%%j
for /f "tokens=1,2" %%i in (rev2.txt) do if %%i==Revision: set TUVOKVERSION=%%j
for /f "tokens=2,3" %%i in (ImageVis3D\StdDefines.h) do if %%i==IV3D_MAJOR set IV3D_MAJOR=%%j
for /f "tokens=2,3" %%i in (ImageVis3D\StdDefines.h) do if %%i==IV3D_MINOR set IV3D_MINOR=%%j
for /f "tokens=2,3" %%i in (ImageVis3D\StdDefines.h) do if %%i==IV3D_PATCH set IV3D_PATCH=%%j
for /f "tokens=2,3" %%i in (Tuvok\StdTuvokDefines.h) do if %%i==TUVOK_MAJOR set TUVOK_MAJOR=%%j
for /f "tokens=2,3" %%i in (Tuvok\StdTuvokDefines.h) do if %%i==TUVOK_MINOR set TUVOK_MINOR=%%j
for /f "tokens=2,3" %%i in (Tuvok\StdTuvokDefines.h) do if %%i==TUVOK_PATCH set TUVOK_PATCH=%%j

set TUVOKCODEVERSION=%TUVOK_MAJOR%.%TUVOK_MINOR%.%TUVOK_PATCH%
set IV3DCODEVERSION=%IV3D_MAJOR%.%IV3D_MINOR%.%IV3D_PATCH%

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

echo Deleting previous binaries >> result.txt
del "Build\Win32\%CONFIG%\ImageVis3D-32.exe"
del "CmdLineConverter\Build\v\%CONFIG%\TuvokConverter32.exe"
del "UVFReader\Build\Win32\%CONFIG%\TuvokReader32.exe"
del "Build\x64\%CONFIG%\ImageVis3D-64.exe"
del "CmdLineConverter\Build\x64\%CONFIG%\TuvokConverter64.exe"
del "UVFReader\Build\x64\%CONFIG%\TuvokReader64.exe"

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

IF EXIST "CmdLineConverter\Build\x64\%CONFIG%\TuvokConverter64.exe" (
  set BUILDUVF64=TRUE
  echo TuvokConverter64 completed >> result.txt
) ELSE (
  set BUILDUVF64=FALSE
  echo TuvokConverter64 failed >> result.txt
)

IF EXIST "UVFReader\Build\x64\%CONFIG%\TuvokReader64.exe" (
  set BUILDUVFR64=TRUE
  echo TuvokReader64 completed >> result.txt
) ELSE (
  set BUILDUVFR64=FALSE
  echo TuvokReader64 failed >> result.txt
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

IF EXIST "CmdLineConverter\Build\Win32\%CONFIG%\TuvokConverter32.exe" (
  set BUILDUVF32=TRUE
  echo TuvokConverter32 completed >> result.txt
) ELSE (
  set BUILDUVF32=FALSE
  echo TuvokConverter32 failed >> result.txt
)

IF EXIST "UVFReader\Build\Win32\%CONFIG%\TuvokReader32.exe" (
  set BUILDUVFR32=TRUE
  echo TuvokReader32 completed >> result.txt
) ELSE (
  set BUILDUVFR32=FALSE
  echo TuvokReader32 failed >> result.txt
)

IF NOT EXIST \\geronimo\share\IV3D-WIN\nul mkdir \\geronimo\share\IV3D-WIN

if NOT !BUILD64!==TRUE (
  if NOT !BUILD32!==TRUE goto ALLFAILED
)

time /t  >> result.txt
echo Packing ZIP file >> result.txt

mkdir Nightly
cd Nightly
if !BUILD64!==TRUE xcopy /y "..\Build\x64\%CONFIG%\ImageVis3D-64.exe" .
if !BUILD32!==TRUE xcopy /y "..\Build\Win32\%CONFIG%\ImageVis3D-32.exe" .
mkdir Shaders
xcopy /Y ..\Tuvok\Shaders\*.glsl .\Shaders

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
echo %TUVOKCODEVERSION% >> Windows_Latest_Version.txt
echo %TUVOKVERSION% >>     Windows_Latest_Version.txt
xcopy Windows_Latest_Version.txt \\geronimo\share\IV3D-WIN /Y
del Windows_Latest_Version.txt


if NOT !BUILDUVF64!==TRUE (
  if NOT !BUILDUVF32!==TRUE goto UVFAILED
)

if !BUILDUVF64!==TRUE xcopy /y "..\CmdLineConverter\Build\x64\%CONFIG%\*.exe" .
if !BUILDUVF32!==TRUE xcopy /y "..\CmdLineConverter\Build\Win32\%CONFIG%\*.exe" .
if !BUILDUVFR64!==TRUE xcopy /y "..\UVFReader\Build\x64\%CONFIG%\*.exe" .
if !BUILDUVFR32!==TRUE xcopy /y "..\UVFReader\Build\Win32\%CONFIG%\*.exe" .

"C:\Program Files\7-Zip\7z" a -r ..\TuvokCmdlineTools_Win_r%REVSTR%.zip
del . /F /S /Q

xcopy ..\ImageVis3D_%IV3DCODEVERSION%_Win_r%REVSTR%.zip \\geronimo\share\IV3D-WIN /Y
del ..\ImageVis3D_%IV3DCODEVERSION%_Win_r%REVSTR%.zip

if !BUILD32!==TRUE (
  echo f | xcopy ..\Scripts\installer\ImageVis3D-32bit.exe \\geronimo\share\IV3D-WIN\ImageVis3D_%IV3DCODEVERSION%_Win_r%REVSTR%-32bit-installer.exe /Y
  echo f | xcopy ..\Scripts\installer\ImageVis3D-32bit.exe \\geronimo\share\IV3D-WIN\ImageVis3D-Latest-32bit-installer.exe /Y
  del ..\Scripts\installer\ImageVis3D-32bit.exe
)

if !BUILD64!==TRUE (
  echo f | xcopy ..\Scripts\installer\ImageVis3D-64bit.exe \\geronimo\share\IV3D-WIN\ImageVis3D_%IV3DCODEVERSION%_Win_r%REVSTR%-64bit-installer.exe /Y
  echo f | xcopy ..\Scripts\installer\ImageVis3D-64bit.exe \\geronimo\share\IV3D-WIN\ImageVis3D-Latest-64bit-installer.exe /Y
  del ..\Scripts\installer\ImageVis3D-64bit.exe
)


IF EXIST ..\TuvokCmdlineTools_Win_r%REVSTR%.zip (
  time /t  >> ..\result.txt
  echo Command Line tools build successful >> ..\result.txt
) else (
  goto ZIPFAILUVF
)

xcopy ..\TuvokCmdlineTools_Win_r%REVSTR%.zip \\geronimo\share\IV3D-WIN /Y
del ..\TuvokCmdlineTools_Win_r%REVSTR%.zip


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

echo Packing the final TuvokConverter ZIP file failed  >> result.txt
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
