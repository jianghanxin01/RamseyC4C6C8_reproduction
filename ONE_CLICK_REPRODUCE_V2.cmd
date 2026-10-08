@echo off
setlocal EnableExtensions DisableDelayedExpansion

rem Extract the complete ZIP, then double-click this file on Windows x64.
rem The default runs all nine cases; --case ID selects one proof for a quick test.
set "RAMSEY_NO_PAUSE=0"
set "RAMSEY_CASE="
set "RAMSEY_EXIT=2"
set "RAMSEY_FAILED_STEP="

:parse
if "%~1"=="" goto parsed
if /i "%~1"=="--no-pause" goto no_pause
if /i "%~1"=="--case" goto select_case
goto usage

:no_pause
set "RAMSEY_NO_PAUSE=1"
shift /1
goto parse

:select_case
if "%~2"=="" goto usage
if defined RAMSEY_CASE goto usage
set "RAMSEY_CASE=%~2"
shift /1
shift /1
goto parse

:parsed
echo ============================================================
echo R(C4,C6,C8) = 12: public reproduction package v2
echo ============================================================
echo Package: "%~dp0"
echo.

echo [1/3] Checking distributed file hashes...
set "RAMSEY_FAILED_STEP=file hash check"
call "%~dp0CHECK_HASHES.cmd" --no-pause
if not "%errorlevel%"=="0" goto failed
echo.

if defined RAMSEY_CASE goto selected
echo [2/3] Checking all nine archived certificates...
set "RAMSEY_FAILED_STEP=archived certificate check"
call "%~dp0VERIFY_ALL.cmd" --no-pause
if not "%errorlevel%"=="0" goto failed
echo.
echo [3/3] Regenerating all nine inputs and proofs, then checking the new proofs...
set "RAMSEY_FAILED_STEP=full regeneration and new certificate check"
call "%~dp0REPRODUCE_ALL.cmd" --no-pause
if not "%errorlevel%"=="0" goto failed
goto success

:selected
echo [2/3] Auditing all nine inputs and checking archived proof %RAMSEY_CASE%...
set "RAMSEY_FAILED_STEP=selected archived certificate check"
call "%~dp0VERIFY_ALL.cmd" --no-pause --case "%RAMSEY_CASE%"
if not "%errorlevel%"=="0" goto failed
echo.
echo [3/3] Regenerating all nine inputs and proof %RAMSEY_CASE%, then checking it...
set "RAMSEY_FAILED_STEP=selected regeneration and new certificate check"
call "%~dp0REPRODUCE_ALL.cmd" --no-pause --case "%RAMSEY_CASE%"
if not "%errorlevel%"=="0" goto failed

:success
set "RAMSEY_EXIT=0"
echo.
echo SUCCESS: the requested computational checks passed.
echo Detailed results are in "%~dp0results".
goto finish

:failed
set "RAMSEY_EXIT=1"
echo.
echo FAILED: %RAMSEY_FAILED_STEP%.
echo No complete reproduction is claimed. Inspect "%~dp0results".
goto finish

:usage
echo Usage: ONE_CLICK_REPRODUCE_V2.cmd [--no-pause] [--case ID]
echo Default: audit and verify all nine archived instances, regenerate all
echo nine inputs and proofs, and independently check all nine new proofs.

:finish
echo.
if "%RAMSEY_NO_PAUSE%"=="0" pause
exit /b %RAMSEY_EXIT%
