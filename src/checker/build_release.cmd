@echo off
setlocal
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if not exist "%VSWHERE%" (
  echo Visual Studio Installer vswhere.exe was not found.
  exit /b 2
)
for /f "usebackq tokens=*" %%i in (`"%VSWHERE%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do set "RAMSEY_VS=%%i"
if not defined RAMSEY_VS (
  echo Install the Visual Studio Desktop development with C++ workload.
  exit /b 2
)
call "%RAMSEY_VS%\VC\Auxiliary\Build\vcvars64.bat"
if errorlevel 1 exit /b 2
pushd "%~dp0"
cl /nologo /std:c++14 /O2 /MT /EHsc /W4 /D_CRT_SECURE_NO_WARNINGS RamseyRup.cpp /Fe:RamseyRup.exe /Fo:RamseyRup.obj /link /INCREMENTAL:NO
set "RAMSEY_BUILD_EXIT=%errorlevel%"
if not "%RAMSEY_BUILD_EXIT%"=="0" (
  popd
  exit /b %RAMSEY_BUILD_EXIT%
)
dumpbin /dependents RamseyRup.exe
popd
exit /b 0
