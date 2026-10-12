@echo off
rem Compila y corre perfiles_test (los 32 Datos personales de PES) con MSVC. Trabaja SOLO sobre una COPIA del archivo.
rem Uso: pruebas-link\correr_perfiles.bat "D:\...\save\SYSTEM00000000"
setlocal
set "R=%~dp0.."
set "T=%TEMP%\phx_perfiles"
set "C=%R%\SmashSoda\phoenix\terceros\pesxcrypter"
where cl >nul 2>&1
if errorlevel 1 call "%ProgramFiles(x86)%\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul
if not exist "%T%" mkdir "%T%"
set "RUN=%T%\corrida-%RANDOM%"
cl /nologo /c /TC /W1 "%C%\crypt.c" "%C%\masterkey.c" "%C%\mt19937ar.c" "%C%\pesxvalidar.c" /Fo"%T%\\" || exit /b 1
cl /nologo /std:c++17 /EHsc /utf-8 /I"%R%\SmashSoda" /I"%R%\SmashSoda\phoenix" /I"%R%\external" "%R%\pruebas-link\perfiles_test.cpp" "%R%\SmashSoda\phoenix\core\PerfilesPes.cpp" "%T%\crypt.obj" "%T%\masterkey.obj" "%T%\mt19937ar.obj" "%T%\pesxvalidar.obj" /Fo"%T%\\" /Fe"%T%\perfiles_test.exe" || exit /b 1
"%T%\perfiles_test.exe" "%RUN%" %1
exit /b %errorlevel%
