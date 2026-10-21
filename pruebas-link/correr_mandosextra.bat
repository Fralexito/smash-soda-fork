@echo off
rem Compila y corre la prueba de phoenix_mandos_extra.dat (mandos 9+) con MSVC. Usa una carpeta temporal: no toca PES.
rem Uso: pruebas-link\correr_mandosextra.bat
setlocal
set "R=%~dp0.."
set "T=%TEMP%\phx_mandosextra"
where cl >nul 2>&1
if errorlevel 1 call "%ProgramFiles(x86)%\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul 2>&1
if not exist "%T%" mkdir "%T%"
cl /nologo /std:c++17 /EHsc /utf-8 /W4 /I"%R%\SmashSoda" "%R%\pruebas-link\mandosextra_test.cpp" "%R%\SmashSoda\phoenix\core\MandosExtra.cpp" shell32.lib ole32.lib /Fo"%T%\\" /Fe"%T%\mandosextra_test.exe" >"%T%\cl.txt" || (type "%T%\cl.txt" & echo mandosextra_test: NO COMPILA & exit /b 1)
"%T%\mandosextra_test.exe" "%T%\run"
if errorlevel 1 (echo mandosextra_test: FALLA & exit /b 1)
echo mandosextra_test: TODO OK
