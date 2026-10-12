@echo off
rem Compila y corre las pruebas de los 32 Datos personales de PES (perfiles_test) y de la base de perfiles de amigos
rem (perfiles_amigos_test) con MSVC. Trabajan SOLO sobre una COPIA del archivo.
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
set "OBJ=%T%\crypt.obj %T%\masterkey.obj %T%\mt19937ar.obj %T%\pesxvalidar.obj"
cl /nologo /std:c++17 /EHsc /utf-8 /I"%R%\SmashSoda" /I"%R%\SmashSoda\phoenix" /I"%R%\external" "%R%\pruebas-link\perfiles_test.cpp" "%R%\SmashSoda\phoenix\core\PerfilesPes.cpp" %OBJ% /Fo"%T%\\" /Fe"%T%\perfiles_test.exe" || exit /b 1
cl /nologo /std:c++17 /EHsc /utf-8 /I"%R%\SmashSoda" /I"%R%\SmashSoda\phoenix" /I"%R%\external" "%R%\pruebas-link\perfiles_amigos_test.cpp" "%R%\SmashSoda\phoenix\core\PerfilesAmigos.cpp" "%R%\SmashSoda\phoenix\core\OpcionesUsuario.cpp" "%R%\SmashSoda\phoenix\core\PerfilesPes.cpp" %OBJ% /Fo"%T%\\" /Fe"%T%\perfiles_amigos_test.exe" || exit /b 1
"%T%\perfiles_test.exe" "%RUN%-a" %1 | findstr /v /b "Usuario" || exit /b 1
"%T%\perfiles_amigos_test.exe" "%RUN%-b" %1
exit /b %errorlevel%
