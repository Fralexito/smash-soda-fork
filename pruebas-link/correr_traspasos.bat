@echo off
rem Compila y corre las pruebas de «Traspasos en vivo» (traspasos_test.cpp) en Windows, con MSVC.
rem Usa carpetas temporales con juegos FALSOS. Con un argumento (una COPIA de EDIT00000000) prueba tambien la cadena completa.
rem Uso: pruebas-link\correr_traspasos.bat ["D:\...\save\EDIT00000000" ["D:\...\Conmegol Patch"]]   (2.o: carpeta del juego real; solo se COPIAN sus bases)
setlocal
set "R=%~dp0.."
set "T=%TEMP%\phx_traspasos"
set "C=%R%\SmashSoda\phoenix\terceros\pesxcrypter"
where cl >nul 2>&1
if errorlevel 1 call "%ProgramFiles(x86)%\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul
if not exist "%T%" mkdir "%T%"
set "RUN=%T%\corrida-%RANDOM%"
cl /nologo /c /TC /W1 "%C%\crypt.c" "%C%\masterkey.c" "%C%\mt19937ar.c" "%C%\pesxvalidar.c" /Fo"%T%\\" || exit /b 1
cl /nologo /std:c++17 /EHsc /utf-8 /I"%R%\SmashSoda" /I"%R%\SmashSoda\phoenix" /I"%R%\external" "%R%\pruebas-link\traspasos_test.cpp" "%R%\SmashSoda\phoenix\link\Captura.cpp" "%R%\SmashSoda\phoenix\link\Traspasos.cpp" "%R%\SmashSoda\phoenix\link\Fichajes.cpp" "%R%\SmashSoda\phoenix\link\ModulosJuego.cpp" "%R%\SmashSoda\phoenix\link\Entrega.cpp" "%R%\SmashSoda\phoenix\link\BuzonJuego.cpp" "%T%\crypt.obj" "%T%\masterkey.obj" "%T%\mt19937ar.obj" "%T%\pesxvalidar.obj" /Fo"%T%\\" /Fe"%T%\traspasos_test.exe" || exit /b 1
"%T%\traspasos_test.exe" "%RUN%" %1 %2
exit /b %errorlevel%
