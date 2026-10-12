@echo off
rem Compila usuariopad.exe (herramienta de investigacion, solo lee) en C:\Temp\usuariopad
where cl >nul 2>&1
if errorlevel 1 call "%ProgramFiles(x86)%\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul
if not exist C:\Temp\usuariopad mkdir C:\Temp\usuariopad
cd /d C:\Temp\usuariopad
cl /nologo /O2 /EHsc /std:c++17 "%~dp0usuariopad.cpp" /Fe:usuariopad.exe
