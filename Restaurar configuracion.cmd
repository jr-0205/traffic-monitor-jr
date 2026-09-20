@echo off
setlocal
set "PS=%SystemRoot%\System32\WindowsPowerShell\v1.0\powershell.exe"
"%PS%" -NoProfile -ExecutionPolicy Bypass -File "%~dp0Aplicar mejoras.ps1" -Mode Restore
set "RC=%ERRORLEVEL%"
if not "%RC%"=="0" pause
exit /b %RC%
