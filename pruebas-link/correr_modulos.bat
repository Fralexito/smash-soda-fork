@echo off
rem Compila y corre las pruebas del instalador de modulos (modulos_test, parche27_test, quitar_modulo_test) con MSVC.
rem Usan carpetas temporales con juegos FALSOS y un sider.ini de ejemplo: no tocan ningun juego real.
rem (Si les pasas un sider.ini REAL como argumento, ese ya debe estar sin las lineas de Phoenix: el del juego de Fralex las tiene desde que se instalo Phoenix.)
rem Uso: pruebas-link\correr_modulos.bat
setlocal
set "R=%~dp0.."
set "T=%TEMP%\phx_modulos"
where cl >nul 2>&1
if errorlevel 1 call "%ProgramFiles(x86)%\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul 2>&1
if not exist "%T%" mkdir "%T%"
set "INI=%T%\sider_ejemplo.ini"
> "%INI%" echo [sider]
>> "%INI%" echo cpk.root = ".\livecpk\Phoenix-DB"
>> "%INI%" echo cpk.root = ".\olmosjr23\Database"
>> "%INI%" echo lua.module = "CGP_data.lua"
set "FALLOS=0"
for %%P in (modulos_test parche27_test quitar_modulo_test) do call :uno %%P
exit /b %FALLOS%

:uno
set "RUN=%T%\run-%1-%RANDOM%"
cl /nologo /std:c++17 /EHsc /utf-8 /I"%R%\SmashSoda" /I"%R%\external" "%R%\pruebas-link\%1.cpp" "%R%\SmashSoda\phoenix\link\ModulosJuego.cpp" "%R%\SmashSoda\phoenix\link\Entrega.cpp" "%R%\SmashSoda\phoenix\link\BuzonJuego.cpp" /Fo"%T%\\" /Fe"%T%\%1.exe" >nul || (echo %1: NO COMPILA & set "FALLOS=1" & goto :eof)
if "%1"=="parche27_test" ("%T%\%1.exe" "%RUN%") else ("%T%\%1.exe" "%RUN%" "%INI%")
if errorlevel 1 (echo %1: FALLA & set "FALLOS=1") else echo %1: TODO OK
goto :eof
