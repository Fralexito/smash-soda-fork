@echo off
rem Phoenix Launcher - abre la ventana para encender/apagar partes del parche y jugar.
rem Fuente: D:\Frank\IA\CLAUDE\mi-parche\launcher\
set "PHX=D:\Frank\IA\CLAUDE\mi-parche\launcher\phoenix_launcher.py"
where pythonw >nul 2>nul && (start "" pythonw "%PHX%" & exit /b)
where pyw >nul 2>nul && (start "" pyw -3 "%PHX%" & exit /b)
echo No encuentro Python. Instala Python 3 o avisa a Claude.
pause
