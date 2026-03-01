@echo off
REM GuessIOServer Setup Script (Batch version)
REM This script automates the setup process for LOCAL DEVELOPMENT ONLY
REM 
REM NOTE: For Docker deployment, the server is automatically built via Dockerfile.
REM This script is only needed if you want to build and run the server locally.

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
