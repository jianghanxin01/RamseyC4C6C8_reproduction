@echo off
setlocal EnableExtensions DisableDelayedExpansion
set "RAMSEY_NO_PAUSE=0"
if "%~1"=="--no-pause" set "RAMSEY_NO_PAUSE=1"
if not "%~1"=="" if not "%~1"=="--no-pause" goto bad_usage
if not "%~2"=="" goto bad_usage
powershell.exe -NoLogo -NoProfile -NonInteractive -ExecutionPolicy Bypass -File "%~dp0CHECK_HASHES.ps1" -Root "%~dp0."
set "RAMSEY_EXIT=%errorlevel%"
goto finish
:bad_usage
echo Usage: CHECK_HASHES.cmd [--no-pause]
set "RAMSEY_EXIT=2"
:finish
if "%RAMSEY_NO_PAUSE%"=="0" pause
exit /b %RAMSEY_EXIT%
