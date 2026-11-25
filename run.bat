@echo off
setlocal

set "PRESET=%~1"
if "%PRESET%"=="" set "PRESET=x64-debug"

call "%~dp0build.bat" %PRESET%
if errorlevel 1 exit /b 1

echo Running SFMLGame...
"%~dp0build\%PRESET%\bin\SFMLGame.exe"
