@echo off
chcp 65001 >nul
cd /d "C:\Users\WinterOS\smash-soda-fork"
powershell -NoProfile -ExecutionPolicy Bypass -File "BUILD_AND_FIX.ps1"
pause
