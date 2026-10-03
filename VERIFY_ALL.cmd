@echo off
setlocal EnableExtensions DisableDelayedExpansion
call "%~dp0RUN_PIPELINE.cmd" verify %*
exit /b %errorlevel%
