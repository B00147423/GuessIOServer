@echo off
REM GuessIOServer Setup Script (Batch version)
REM This script automates the setup process for the project

echo === GuessIOServer Setup ===
echo.

REM Check if PowerShell is available
where powershell >nul 2>&1
if %ERRORLEVEL% NEQ 0 (
    echo PowerShell is required. Please install it and try again.
    exit /b 1
)

REM Run the PowerShell script
powershell -ExecutionPolicy Bypass -File "%~dp0setup.ps1"

if %ERRORLEVEL% NEQ 0 (
    echo Setup failed.
    exit /b 1
)

pause
