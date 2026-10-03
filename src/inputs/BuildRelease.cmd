@echo off
setlocal
rem Build the C++14 program with the installed Visual Studio x64 compiler.
rem Run from Explorer, cmd.exe, PowerShell, or an x64 Native Tools prompt.
where cl.exe >nul 2>nul
if errorlevel 1 (
  if not exist "%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" (
    echo Visual Studio C++ Build Tools were not found.
    exit /b 2
  )
  for /f "usebackq delims=" %%i in (`"%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do set "RI_VS=%%i"
)
if defined RI_VS call "%RI_VS%\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 2
pushd "%~dp0"
if not exist "bin\x64\Release" mkdir "bin\x64\Release"
if not exist "obj\x64\Release" mkdir "obj\x64\Release"
cl.exe /nologo /std:c++14 /O2 /MT /EHsc /W4 /WX /utf-8 /DWIN32_LEAN_AND_MEAN /DNOMINMAX RamseyInputs.cpp /Foobj\x64\Release\RamseyInputs.obj /Febin\x64\Release\RamseyInputs.exe /link /SUBSYSTEM:CONSOLE
set "RI_RESULT=%errorlevel%"
popd
exit /b %RI_RESULT%
