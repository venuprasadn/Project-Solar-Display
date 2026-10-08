@echo off
title Solar PCU OEM Builder Wizard
cd /d "%~dp0"
python configure_and_build.py
if %ERRORLEVEL% NEQ 0 (
    echo.
    echo [ERROR] Builder failed. Check error above.
    pause
)
