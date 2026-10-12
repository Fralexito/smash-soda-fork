@echo off
rem Compila y corre las pruebas de ESCENARIOS de Sync (escenarios_sync_test.cpp) en Windows, con MSVC.
rem Usa carpetas temporales con juegos FALSOS: no toca ningun juego real. Uso: pruebas-link\correr_escenarios.bat
rem Si "cl" no se encuentra, abre antes una "Developer Command Prompt" de Visual Studio y ejecutalo desde ahi.
setlocal
set "R=%~dp0.."
set "T=%TEMP%\phx_escenarios"
where cl >nul 2>&1
if errorlevel 1 call "%ProgramFiles(x86)%\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul
if not exist "%T%" mkdir "%T%"
set "RUN=%T%\corrida-%RANDOM%"
cl /nologo /std:c++17 /EHsc /utf-8 /I"%R%\SmashSoda" /I"%R%\external" "%R%\pruebas-link\escenarios_sync_test.cpp" "%R%\SmashSoda\phoenix\link\ModulosJuego.cpp" "%R%\SmashSoda\phoenix\link\Entrega.cpp" "%R%\SmashSoda\phoenix\link\BuzonJuego.cpp" /Fo"%T%\\" /Fe"%T%\escenarios_sync_test.exe" || exit /b 1
"%T%\escenarios_sync_test.exe" "%RUN%" "%R%\SmashSoda\phoenix\sider"
exit /b %errorlevel%
