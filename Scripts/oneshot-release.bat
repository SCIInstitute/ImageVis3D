REM IV3D release build

set w32_cf="-D_CRT_SECURE_NO_WARNINGS=1 -D_SCL_SECURE_NO_WARNINGS=1"

set bld="msbuild"
IF "%1"=="x64" (
  REM Handle 64 bit compilation
  call "%VS100COMNTOOLS%\..\..\VC\vcvarsall.bat" x64
  %bld% ^
    ImageVis3D-2010.sln ^
    /nologo ^
    /p:Configuration="Release (with DirectX)",Platform=x64 ^
    /t:Build ^
    /m:2
) ELSE (
  REM Handle 32 bit compilation
  call "%VS100COMNTOOLS%\..\..\VC\vcvarsall.bat"
  %bld% ^
    ImageVis3D-2010.sln ^
    /nologo ^
    /p:Configuration="Release (with DirectX)",Platform=Win32 ^
    /t:Build ^
    /m:2
)

REM download documentation
powershell -ExecutionPolicy Unrestricted -file Scripts\dl.ps1

REM bundle it
IF "%1"=="x64" (
  iscc Scripts/installer/64.iss
) ELSE (
  iscc Scripts/installer/32.iss
)

