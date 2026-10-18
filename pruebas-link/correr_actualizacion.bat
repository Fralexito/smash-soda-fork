@echo off
rem Compila y corre las pruebas de la auto-actualizacion (actualizacion_test.cpp) con MSVC. Sin internet ni PES:
rem servidor web falso en 127.0.0.1, clave de prueba, zips de prueba y un «Link falso». Todo en %TEMP%\phx_actualizacion.
setlocal
set "R=%~dp0.."
set "T=%TEMP%\phx_actualizacion"
set "E=%R%\SmashSoda\phoenix\terceros\ed25519"
where cl >nul 2>&1
if errorlevel 1 call "%ProgramFiles(x86)%\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul
if not exist "%T%" mkdir "%T%"
cl /nologo /c /TC /W1 "%E%\fe.c" "%E%\ge.c" "%E%\sc.c" "%E%\sha512.c" "%E%\verify.c" "%E%\sign.c" "%E%\keypair.c" /Fo"%T%\\" || exit /b 1
cl /nologo /std:c++17 /EHsc /utf-8 /DUNICODE /D_UNICODE "%R%\pruebas-link\actualizacion_falso_link.cpp" /Fo"%T%\\" /Fe"%T%\falso_link.exe" /link /SUBSYSTEM:WINDOWS || exit /b 1
cl /nologo /std:c++17 /EHsc /utf-8 /DNOMINMAX /I"%R%\SmashSoda" /I"%R%\external" "%R%\pruebas-link\actualizacion_test.cpp" "%R%\SmashSoda\phoenix\link\Actualizacion.cpp" "%R%\PhoenixActualizador\Instalador.cpp" "%T%\fe.obj" "%T%\ge.obj" "%T%\sc.obj" "%T%\sha512.obj" "%T%\verify.obj" "%T%\sign.obj" "%T%\keypair.obj" /Fo"%T%\\" /Fe"%T%\actualizacion_test.exe" || exit /b 1
"%T%\actualizacion_test.exe" "%T%\corrida" "%T%\falso_link.exe"
exit /b %errorlevel%
