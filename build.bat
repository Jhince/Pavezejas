@echo off
setlocal

set "PRESET=%~1"
if "%PRESET%"=="" set "PRESET=x64-debug"

:: Find Visual Studio installation via vswhere
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if not exist "%VSWHERE%" (
    echo Error: vswhere.exe not found. Please install Visual Studio with C++ Desktop Development.
    exit /b 1
)

for /f "usebackq tokens=*" %%i in (`"%VSWHERE%" -latest -property installationPath`) do set "VS_PATH=%%i"
if not exist "%VS_PATH%\VC\Auxiliary\Build\vcvars64.bat" (
    echo Error: Visual Studio C++ tools ^(vcvars64.bat^) not found.
    exit /b 1
)

:: Initialize MSVC, CMake, and Ninja environment if cl.exe isn't already in PATH
where cl >nul 2>nul
if errorlevel 1 (
    call "%VS_PATH%\VC\Auxiliary\Build\vcvars64.bat" >nul
)

echo Configuring (%PRESET%)...
cmake --preset %PRESET%
if errorlevel 1 exit /b 1

echo Building (%PRESET%)...
cmake --build --preset %PRESET%
if errorlevel 1 exit /b 1

echo.
echo Build succeeded: build\%PRESET%\bin\SFMLGame.exe
