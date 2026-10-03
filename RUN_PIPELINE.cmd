@echo off
setlocal EnableExtensions DisableDelayedExpansion
pushd "%~dp0"
if errorlevel 1 exit /b 2
set "RAMSEY_MODE=%~1"
shift /1
set "RAMSEY_NO_PAUSE=0"
set "RAMSEY_CASE="
set "RAMSEY_BIN=%~dp0bin"
set "RAMSEY_BIN_SELECTED="
set "RAMSEY_EXIT=2"
set "RAMSEY_RUN="
set "RAMSEY_SUMMARY="
set "RAMSEY_STEP=argument parsing"
set "RAMSEY_CHECKED=0"
set "RAMSEY_FAILURE="
set "RAMSEY_IDS=48T1 48T2 H10-I3 H10-K2_K1 H10-P3 B-root-1 B-root-2 B-leafK4 B-C9"

:parse
if "%~1"=="" goto parsed
if /i "%~1"=="--no-pause" goto no_pause
if /i "%~1"=="--case" goto select_case
if /i "%~1"=="--bin" goto select_bin
if /i "%~1"=="--help" goto help
echo ERROR: Unknown argument "%~1".
goto usage_error

:no_pause
set "RAMSEY_NO_PAUSE=1"
shift /1
goto parse

:select_case
if "%~2"=="" goto missing_case
if defined RAMSEY_CASE goto duplicate_case
set "RAMSEY_CASE=%~2"
shift /1
shift /1
goto parse

:missing_case
echo ERROR: --case requires an instance ID.
goto usage_error

:select_bin
if "%~2"=="" goto missing_bin
if defined RAMSEY_BIN_SELECTED goto duplicate_bin
set "RAMSEY_BIN=%~f2"
set "RAMSEY_BIN_SELECTED=1"
shift /1
shift /1
goto parse

:missing_bin
echo ERROR: --bin requires a directory.
goto usage_error

:duplicate_bin
echo ERROR: --bin may be supplied only once.
goto usage_error

:duplicate_case
echo ERROR: --case may be supplied only once.
goto usage_error

:parsed
if not "%RAMSEY_MODE%"=="verify" if not "%RAMSEY_MODE%"=="reproduce" goto usage_error
if not defined RAMSEY_CASE goto case_validated
set "RAMSEY_CASE_VALID=0"
for %%I in (%RAMSEY_IDS%) do if "%RAMSEY_CASE%"=="%%I" set "RAMSEY_CASE_VALID=1"
if "%RAMSEY_CASE_VALID%"=="1" goto case_validated
echo ERROR: Unknown instance ID "%RAMSEY_CASE%".
goto usage_error

:case_validated
set "RAMSEY_STEP=locating programs and data"
if not exist "%RAMSEY_BIN%\RamseyInputs.exe" goto missing_inputs
if not exist "%RAMSEY_BIN%\RamseyRup.exe" goto missing_checker
if "%RAMSEY_MODE%"=="reproduce" if not exist "%RAMSEY_BIN%\RamseySolve.exe" goto missing_solver
if not exist "%~dp0data\" goto missing_data
set "RAMSEY_STAMP="
for /f "delims=" %%T in ('powershell.exe -NoLogo -NoProfile -NonInteractive -Command "Get-Date -Format yyyyMMdd_HHmmss_fff"') do set "RAMSEY_STAMP=%%T"
if not defined RAMSEY_STAMP goto timestamp_error

:new_run_directory
set "RAMSEY_RUN=%~dp0results\%RAMSEY_MODE%_%RAMSEY_STAMP%_%RANDOM%"
if exist "%RAMSEY_RUN%\" goto new_run_directory
mkdir "%RAMSEY_RUN%" >nul 2>&1
set "RAMSEY_STEP_EXIT=%errorlevel%"
if not "%RAMSEY_STEP_EXIT%"=="0" goto directory_error
set "RAMSEY_SUMMARY=%RAMSEY_RUN%\summary.txt"
>"%RAMSEY_SUMMARY%" echo RamseyC4C6C8 reproduction: %RAMSEY_MODE%
>>"%RAMSEY_SUMMARY%" echo Started: %RAMSEY_STAMP%
>>"%RAMSEY_SUMMARY%" echo Executable directory: "%RAMSEY_BIN%"
>>"%RAMSEY_SUMMARY%" echo All nine graph inputs are audited before any proof result is accepted.
echo.
echo RamseyC4C6C8 reproduction: %RAMSEY_MODE%
echo Log directory: "%RAMSEY_RUN%"
echo.
if defined RAMSEY_CASE >>"%RAMSEY_SUMMARY%" echo Selected proof case: %RAMSEY_CASE%
if not defined RAMSEY_CASE >>"%RAMSEY_SUMMARY%" echo Selected proof cases: all nine
set "RAMSEY_STEP=auditing all nine graph inputs"
echo Auditing all nine graph inputs...
if "%RAMSEY_MODE%"=="reproduce" goto generate_inputs
"%RAMSEY_BIN%\RamseyInputs.exe" --all --data "%~dp0data" --report "%RAMSEY_RUN%\inputs.json" >"%RAMSEY_RUN%\inputs.log" 2>&1
set "RAMSEY_STEP_EXIT=%errorlevel%"
if not "%RAMSEY_STEP_EXIT%"=="0" goto failed
goto inputs_passed

:generate_inputs
set "RAMSEY_STEP=generating and auditing all nine graph inputs"
mkdir "%RAMSEY_RUN%\generated" >nul 2>&1
set "RAMSEY_STEP_EXIT=%errorlevel%"
if not "%RAMSEY_STEP_EXIT%"=="0" goto failed
"%RAMSEY_BIN%\RamseyInputs.exe" --all --data "%~dp0data" --generate "%RAMSEY_RUN%\generated" --report "%RAMSEY_RUN%\inputs.json" >"%RAMSEY_RUN%\inputs.log" 2>&1
set "RAMSEY_STEP_EXIT=%errorlevel%"
if not "%RAMSEY_STEP_EXIT%"=="0" goto failed

:inputs_passed
echo Input audit passed.
>>"%RAMSEY_SUMMARY%" echo INPUT AUDIT PASSED: all nine cases; exit=0
if defined RAMSEY_CASE goto run_selected
for %%I in (%RAMSEY_IDS%) do call :run_case "%%I"
if defined RAMSEY_FAILURE goto failed
goto all_passed

:run_selected
call :run_case "%RAMSEY_CASE%"
if defined RAMSEY_FAILURE goto failed
goto all_passed

:run_case
rem A failed case prevents all subsequent loop iterations from launching work.
if defined RAMSEY_FAILURE exit /b 1
set "RAMSEY_CURRENT=%~1"
call :locate_archived_case "%RAMSEY_CURRENT%"
if "%RAMSEY_MODE%"=="reproduce" goto reproduce_case
set "RAMSEY_CNF=%~dp0data\%RAMSEY_CNF_REL%"
set "RAMSEY_PROOF=%~dp0data\%RAMSEY_PROOF_REL%"
goto check_case

:reproduce_case
set "RAMSEY_CNF=%RAMSEY_RUN%\generated\%RAMSEY_CURRENT%.cnf"
set "RAMSEY_PROOF=%RAMSEY_RUN%\generated\%RAMSEY_CURRENT%.rup"
set "RAMSEY_STEP=solving generated %RAMSEY_CURRENT%.cnf"
echo Solving %RAMSEY_CURRENT%...
"%RAMSEY_BIN%\RamseySolve.exe" "%RAMSEY_CNF%" "%RAMSEY_PROOF%" >"%RAMSEY_RUN%\%RAMSEY_CURRENT%.solve.log" 2>&1
set "RAMSEY_STEP_EXIT=%errorlevel%"
rem Glucose's UNSAT status is 20, not 0. It is not accepted as a proof.
if not "%RAMSEY_STEP_EXIT%"=="20" goto case_failed
>>"%RAMSEY_SUMMARY%" echo SOLVER UNSAT: %RAMSEY_CURRENT%; exit=20; pending independent proof check

:check_case
set "RAMSEY_STEP=checking RUP certificate for %RAMSEY_CURRENT%"
echo Checking RUP certificate for %RAMSEY_CURRENT%...
"%RAMSEY_BIN%\RamseyRup.exe" "%RAMSEY_CNF%" "%RAMSEY_PROOF%" >"%RAMSEY_RUN%\%RAMSEY_CURRENT%.rup.log" 2>&1
set "RAMSEY_STEP_EXIT=%errorlevel%"
if not "%RAMSEY_STEP_EXIT%"=="0" goto case_failed
set /a RAMSEY_CHECKED+=1 >nul
>>"%RAMSEY_SUMMARY%" echo CERTIFICATE PASSED: %RAMSEY_CURRENT%; checker exit=0
>>"%RAMSEY_SUMMARY%" echo   CNF: "%RAMSEY_CNF%"
>>"%RAMSEY_SUMMARY%" echo   RUP: "%RAMSEY_PROOF%"
echo Passed: %RAMSEY_CURRENT%
exit /b 0

:case_failed
set "RAMSEY_FAILURE=1"
exit /b 1

:locate_archived_case
if "%~1"=="48T1" set "RAMSEY_CNF_REL=arrows\certificates\48T1.cnf"
if "%~1"=="48T1" set "RAMSEY_PROOF_REL=arrows\certificates\48T1.rup"
if "%~1"=="48T2" set "RAMSEY_CNF_REL=arrows\certificates\48T2.cnf"
if "%~1"=="48T2" set "RAMSEY_PROOF_REL=arrows\certificates\48T2.rup"
if "%~1"=="H10-I3" set "RAMSEY_CNF_REL=hexagon\hexagon_refined52\I3.cnf"
if "%~1"=="H10-I3" set "RAMSEY_PROOF_REL=hexagon\hexagon_refined52\I3.rup"
if "%~1"=="H10-K2_K1" set "RAMSEY_CNF_REL=hexagon\hexagon_refined52\K2_K1.cnf"
if "%~1"=="H10-K2_K1" set "RAMSEY_PROOF_REL=hexagon\hexagon_refined52\K2_K1.rup"
if "%~1"=="H10-P3" set "RAMSEY_CNF_REL=hexagon\hexagon_refined52\P3.cnf"
if "%~1"=="H10-P3" set "RAMSEY_PROOF_REL=hexagon\hexagon_refined52\P3.rup"
if "%~1"=="B-root-1" set "RAMSEY_CNF_REL=blue\blue_d1_full53.cnf"
if "%~1"=="B-root-1" set "RAMSEY_PROOF_REL=blue\blue_d1_full53.rup"
if "%~1"=="B-root-2" set "RAMSEY_CNF_REL=blue\blue_d2_full53.cnf"
if "%~1"=="B-root-2" set "RAMSEY_PROOF_REL=blue\blue_d2_full53.rup"
if "%~1"=="B-leafK4" set "RAMSEY_CNF_REL=blue\blue_high_leafK4\instance.cnf"
if "%~1"=="B-leafK4" set "RAMSEY_PROOF_REL=blue\blue_high_leafK4\proof.rup"
if "%~1"=="B-C9" set "RAMSEY_CNF_REL=blue\blue_high_C9\instance.cnf"
if "%~1"=="B-C9" set "RAMSEY_PROOF_REL=blue\blue_high_C9\proof.rup"
exit /b 0

:all_passed
set "RAMSEY_EXIT=0"
echo.
echo SUCCESS: all nine input audits and %RAMSEY_CHECKED% selected certificate check(s) passed.
>>"%RAMSEY_SUMMARY%" echo SUCCESS: all nine input audits and %RAMSEY_CHECKED% selected certificate check(s) passed.
goto finish

:failed
set "RAMSEY_EXIT=1"
echo.
echo FAILED: %RAMSEY_STEP%; executable exit=%RAMSEY_STEP_EXIT%.
echo No overall success is claimed. See the logs for the failing step.
if defined RAMSEY_SUMMARY >>"%RAMSEY_SUMMARY%" echo FAILED: %RAMSEY_STEP%; executable exit=%RAMSEY_STEP_EXIT%
if defined RAMSEY_SUMMARY >>"%RAMSEY_SUMMARY%" echo No overall success is claimed. Certificates completed before failure: %RAMSEY_CHECKED%
goto finish

:missing_inputs
echo ERROR: Missing "%RAMSEY_BIN%\RamseyInputs.exe".
goto finish
:missing_checker
echo ERROR: Missing "%RAMSEY_BIN%\RamseyRup.exe".
goto finish
:missing_solver
echo ERROR: Missing "%RAMSEY_BIN%\RamseySolve.exe".
goto finish
:missing_data
echo ERROR: Missing data directory.
goto finish
:timestamp_error
echo ERROR: Could not obtain the timestamp using Windows PowerShell.
goto finish
:directory_error
echo ERROR: Could not create a fresh log directory.
goto finish

:help
set "RAMSEY_EXIT=0"
echo Usage: VERIFY_ALL.cmd [--no-pause] [--case ID] [--bin DIR]
echo        REPRODUCE_ALL.cmd [--no-pause] [--case ID] [--bin DIR]
echo.
echo Both commands audit all nine inputs first.
echo --case selects one certificate to check or one new proof to produce.
echo --bin selects the executable directory; relative paths use this script's directory.
echo IDs: %RAMSEY_IDS%
goto finish

:usage_error
set "RAMSEY_EXIT=2"
echo Use --help for command syntax and case IDs.

:finish
if defined RAMSEY_RUN echo Results: "%RAMSEY_RUN%"
popd
if "%RAMSEY_NO_PAUSE%"=="0" pause
exit /b %RAMSEY_EXIT%
