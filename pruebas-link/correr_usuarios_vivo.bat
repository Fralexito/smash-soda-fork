@echo off
rem Compila y corre usuarios_vivo_test (SOLO LEE): Usuario de cada mando y nombres de la tabla de un PES 2021 ABIERTO.
setlocal
set "R=%~dp0.."
set "T=%TEMP%\phx_usuarios_vivo"
set "C=%R%\SmashSoda\phoenix\terceros\pesxcrypter"
where cl >nul 2>&1
if errorlevel 1 call "%ProgramFiles(x86)%\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul
if not exist "%T%" mkdir "%T%"
cl /nologo /std:c++17 /EHsc /utf-8 /I"%R%\SmashSoda" /I"%R%\SmashSoda\phoenix" /I"%R%\external" "%R%\pruebas-link\usuarios_vivo_test.cpp" "%R%\SmashSoda\phoenix\core\UsuariosEnVivo.cpp" "%R%\SmashSoda\phoenix\core\PerfilesPes.cpp" /c /Fo"%T%\\" || exit /b 1
cl /nologo /c /TC /W1 "%C%\crypt.c" "%C%\masterkey.c" "%C%\mt19937ar.c" "%C%\pesxvalidar.c" /Fo"%T%\\" || exit /b 1
link /nologo "%T%\usuarios_vivo_test.obj" "%T%\UsuariosEnVivo.obj" "%T%\PerfilesPes.obj" "%T%\crypt.obj" "%T%\masterkey.obj" "%T%\mt19937ar.obj" "%T%\pesxvalidar.obj" /out:"%T%\usuarios_vivo_test.exe" || exit /b 1
"%T%\usuarios_vivo_test.exe"
exit /b %errorlevel%
