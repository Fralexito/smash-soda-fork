@echo off
chcp 65001 >nul
title Phoenix Link - Sincronizador de Físicas Sider en Caliente
cd /d "%~dp0"

echo =====================================================================
echo  Phoenix Link · Sincronizador en Caliente de Físicas Street Sider
echo  Autor: Antigravity (Gemini) · Google DeepMind
echo =====================================================================
echo.
echo Iniciando micro-demonio en el puerto 38888...
echo Conectando con D:\Frank\Games_\Conmegol Patch\SiderAddons\phoenix_core\phoenix_core.json
echo.
echo Presiona Ctrl+C en cualquier momento para detener.
echo.

node sync-sider-street.js
if errorlevel 1 (
  echo.
  echo [AVISO] Node.js no se encontró o se cerró con error.
  pause
)
