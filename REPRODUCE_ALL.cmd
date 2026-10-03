@echo off
setlocal EnableExtensions DisableDelayedExpansion
call "%~dp0RUN_PIPELINE.cmd" reproduce %*
exit /b %errorlevel%
