@echo off
rem Compila y corre las pruebas de «fichajes como cambios» (fichajes_test.cpp) en Windows, con MSVC.
rem Usa carpetas temporales con juegos FALSOS. Con un argumento (carpeta del juego real) prueba ademas con COPIAS de sus bases (solo lectura).
rem Uso: pruebas-link\correr_fichajes.bat ["D:\...\Conmegol Patch"]
setlocal
set "R=%~dp0.."
set "T=%TEMP%\phx_fichajes"
where cl >nul 2>&1
if errorlevel 1 call "%ProgramFiles(x86)%\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul
if not exist "%T%" mkdir "%T%"
set "RUN=%T%\corrida-%RANDOM%"
cl /nologo /std:c++17 /EHsc /utf-8 /I"%R%\SmashSoda" /I"%R%\external" "%R%\pruebas-link\fichajes_test.cpp" "%R%\SmashSoda\phoenix\link\Fichajes.cpp" "%R%\SmashSoda\phoenix\link\ModulosJuego.cpp" "%R%\SmashSoda\phoenix\link\Entrega.cpp" "%R%\SmashSoda\phoenix\link\BuzonJuego.cpp" /Fo"%T%\\" /Fe"%T%\fichajes_test.exe" || exit /b 1
"%T%\fichajes_test.exe" "%RUN%" %1
exit /b %errorlevel%
