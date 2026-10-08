@echo off
setlocal EnableExtensions DisableDelayedExpansion
pushd "%~dp0"
if errorlevel 1 exit /b 2
set "RAMSEY_NO_PAUSE=0"
set "RAMSEY_BUILD_EXIT=2"
set "RAMSEY_BUILD_LOGS="
if "%~1"=="--no-pause" set "RAMSEY_NO_PAUSE=1"
if "%~1"=="--help" goto help
if not "%~1"=="" if not "%~1"=="--no-pause" goto usage_error
if not "%~2"=="" goto usage_error
if not exist "%~dp0src\inputs\BuildRelease.cmd" goto missing_source
if not exist "%~dp0src\checker\build_release.cmd" goto missing_source
if not exist "%~dp0src\solver\build_release.cmd" goto missing_source
set "RAMSEY_STAMP="
for /f "delims=" %%T in ('powershell.exe -NoLogo -NoProfile -NonInteractive -Command "Get-Date -Format yyyyMMdd_HHmmss_fff"') do set "RAMSEY_STAMP=%%T"
if not defined RAMSEY_STAMP goto timestamp_error

:new_build_directory
set "RAMSEY_BUILD_LOGS=%~dp0results\build_%RAMSEY_STAMP%_%RANDOM%"
if exist "%RAMSEY_BUILD_LOGS%\" goto new_build_directory
mkdir "%RAMSEY_BUILD_LOGS%" >nul 2>&1
set "RAMSEY_COMPONENT_EXIT=%errorlevel%"
if not "%RAMSEY_COMPONENT_EXIT%"=="0" goto directory_error
>"%RAMSEY_BUILD_LOGS%\summary.txt" echo Ramsey reproduction v2 source build
>>"%RAMSEY_BUILD_LOGS%\summary.txt" echo Started: %RAMSEY_STAMP%
echo Build logs: "%RAMSEY_BUILD_LOGS%"
set "RAMSEY_BUILD_STEP=copying source to the fresh build directory"
xcopy "%~dp0src" "%RAMSEY_BUILD_LOGS%\src" /E /I /Q /Y >"%RAMSEY_BUILD_LOGS%\source_copy.log" 2>&1
set "RAMSEY_COMPONENT_EXIT=%errorlevel%"
if not "%RAMSEY_COMPONENT_EXIT%"=="0" goto failed

set "RAMSEY_BUILD_STEP=building RamseyInputs"
echo Building RamseyInputs...
call "%RAMSEY_BUILD_LOGS%\src\inputs\BuildRelease.cmd" >"%RAMSEY_BUILD_LOGS%\inputs_build.log" 2>&1
set "RAMSEY_COMPONENT_EXIT=%errorlevel%"
if not "%RAMSEY_COMPONENT_EXIT%"=="0" goto failed
>>"%RAMSEY_BUILD_LOGS%\summary.txt" echo RamseyInputs build passed; exit=0

set "RAMSEY_BUILD_STEP=building RamseyRup"
echo Building RamseyRup...
call "%RAMSEY_BUILD_LOGS%\src\checker\build_release.cmd" >"%RAMSEY_BUILD_LOGS%\checker_build.log" 2>&1
set "RAMSEY_COMPONENT_EXIT=%errorlevel%"
if not "%RAMSEY_COMPONENT_EXIT%"=="0" goto failed
>>"%RAMSEY_BUILD_LOGS%\summary.txt" echo RamseyRup build passed; exit=0

set "RAMSEY_BUILD_STEP=building RamseySolve"
echo Building RamseySolve...
call "%RAMSEY_BUILD_LOGS%\src\solver\build_release.cmd" >"%RAMSEY_BUILD_LOGS%\solver_build.log" 2>&1
set "RAMSEY_COMPONENT_EXIT=%errorlevel%"
if not "%RAMSEY_COMPONENT_EXIT%"=="0" goto failed
>>"%RAMSEY_BUILD_LOGS%\summary.txt" echo RamseySolve build passed; exit=0

set "RAMSEY_BUILD_STEP=copying compiled programs to rebuilt_bin"
if exist "%~dp0rebuilt_bin\" goto copy_programs
mkdir "%~dp0rebuilt_bin" >nul 2>&1
set "RAMSEY_COMPONENT_EXIT=%errorlevel%"
if not "%RAMSEY_COMPONENT_EXIT%"=="0" goto failed
:copy_programs
copy /Y "%RAMSEY_BUILD_LOGS%\src\inputs\bin\x64\Release\RamseyInputs.exe" "%~dp0rebuilt_bin\RamseyInputs.exe" >"%RAMSEY_BUILD_LOGS%\copy_binaries.log" 2>&1
set "RAMSEY_COMPONENT_EXIT=%errorlevel%"
if not "%RAMSEY_COMPONENT_EXIT%"=="0" goto failed
copy /Y "%RAMSEY_BUILD_LOGS%\src\checker\RamseyRup.exe" "%~dp0rebuilt_bin\RamseyRup.exe" >>"%RAMSEY_BUILD_LOGS%\copy_binaries.log" 2>&1
set "RAMSEY_COMPONENT_EXIT=%errorlevel%"
if not "%RAMSEY_COMPONENT_EXIT%"=="0" goto failed
copy /Y "%RAMSEY_BUILD_LOGS%\src\solver\RamseySolve.exe" "%~dp0rebuilt_bin\RamseySolve.exe" >>"%RAMSEY_BUILD_LOGS%\copy_binaries.log" 2>&1
set "RAMSEY_COMPONENT_EXIT=%errorlevel%"
if not "%RAMSEY_COMPONENT_EXIT%"=="0" goto failed
set "RAMSEY_BUILD_EXIT=0"
echo SUCCESS: three C++14 Release programs built in rebuilt_bin.
echo The shipped bin programs and distributed source files were not changed.
>>"%RAMSEY_BUILD_LOGS%\summary.txt" echo SUCCESS: three C++14 Release programs built in rebuilt_bin; exit=0
goto finish

:failed
set "RAMSEY_BUILD_EXIT=1"
echo FAILED: %RAMSEY_BUILD_STEP%; exit=%RAMSEY_COMPONENT_EXIT%.
>>"%RAMSEY_BUILD_LOGS%\summary.txt" echo FAILED: %RAMSEY_BUILD_STEP%; exit=%RAMSEY_COMPONENT_EXIT%
goto finish
:missing_source
echo ERROR: The src component directories or their build commands are missing.
goto finish
:timestamp_error
echo ERROR: Could not obtain the timestamp using Windows PowerShell.
goto finish
:directory_error
echo ERROR: Could not create a fresh build directory.
goto finish
:help
set "RAMSEY_BUILD_EXIT=0"
echo Usage: BUILD_ALL.cmd [--no-pause]
echo Builds the three native C++14 programs into rebuilt_bin using Visual Studio.
echo Source copies and build logs remain in a fresh results\build_TIMESTAMP_RANDOM folder.
goto finish
:usage_error
echo Usage: BUILD_ALL.cmd [--no-pause]
:finish
if defined RAMSEY_BUILD_LOGS echo Build evidence: "%RAMSEY_BUILD_LOGS%"
popd
if "%RAMSEY_NO_PAUSE%"=="0" pause
exit /b %RAMSEY_BUILD_EXIT%
