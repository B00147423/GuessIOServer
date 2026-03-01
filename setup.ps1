# GuessIOServer Setup Script
# This script automates the setup process for LOCAL DEVELOPMENT ONLY
# 
# NOTE: For Docker deployment, the server is automatically built via Dockerfile.
# This script is only needed if you want to build and run the server locally.

# Ensure we're in the project directory
$scriptPath = Split-Path -Parent $MyInvocation.MyCommand.Path
Push-Location $scriptPath

Write-Host "=== GuessIOServer Setup ===" -ForegroundColor Cyan
Write-Host "Working directory: $scriptPath" -ForegroundColor Gray
Write-Host ""

# Check if vcpkg is installed
$vcpkgPath = $env:VCPKG_ROOT
if (-not $vcpkgPath) {
    Write-Host "vcpkg not found. Installing vcpkg..." -ForegroundColor Yellow
    
    $vcpkgDefaultPath = "C:\vcpkg"
    
    if (-not (Test-Path $vcpkgDefaultPath)) {
        Write-Host "Cloning vcpkg to $vcpkgDefaultPath..." -ForegroundColor Yellow
        git clone https://github.com/Microsoft/vcpkg.git $vcpkgDefaultPath
        
        if ($LASTEXITCODE -ne 0) {
            Write-Host "Failed to clone vcpkg. Please install it manually." -ForegroundColor Red
            exit 1
        }
    }
    
    Write-Host "Bootstrapping vcpkg..." -ForegroundColor Yellow
    Push-Location $vcpkgDefaultPath
    .\bootstrap-vcpkg.bat
    Pop-Location
    
    Write-Host "Setting VCPKG_ROOT environment variable..." -ForegroundColor Yellow
    [System.Environment]::SetEnvironmentVariable("VCPKG_ROOT", $vcpkgDefaultPath, "User")
    $env:VCPKG_ROOT = $vcpkgDefaultPath
    
    Write-Host "vcpkg installed successfully!" -ForegroundColor Green
    Write-Host "NOTE: You may need to restart your terminal/VSCode for the environment variable to take effect." -ForegroundColor Yellow
    Write-Host ""
} else {
    Write-Host "vcpkg found at: $vcpkgPath" -ForegroundColor Green
    Write-Host ""
}

# Install dependencies using vcpkg manifest mode
# Note: vcpkg.json in the project directory will be automatically detected
Write-Host "Installing dependencies (this may take 15-30 minutes)..." -ForegroundColor Yellow
Write-Host "This will install: boost-asio, boost-beast, grpc, protobuf, nlohmann-json" -ForegroundColor Yellow
Write-Host ""

$vcpkgExe = Join-Path $env:VCPKG_ROOT "vcpkg.exe"
if (-not (Test-Path $vcpkgExe)) {
    Write-Host "vcpkg.exe not found at $vcpkgExe" -ForegroundColor Red
    Pop-Location
    exit 1
}

# Check if vcpkg.json exists
if (-not (Test-Path "vcpkg.json")) {
    Write-Host "ERROR: vcpkg.json not found in current directory!" -ForegroundColor Red
    Write-Host "Please run this script from the project root directory." -ForegroundColor Red
    Pop-Location
    exit 1
}

# Install dependencies using manifest mode (vcpkg.json will be auto-detected)
& $vcpkgExe install --triplet x64-windows

if ($LASTEXITCODE -ne 0) {
    Write-Host "Failed to install dependencies." -ForegroundColor Red
    Pop-Location
    exit 1
}

Write-Host ""
Write-Host "Dependencies installed successfully!" -ForegroundColor Green
Write-Host ""

# Configure CMake
Write-Host "Configuring CMake..." -ForegroundColor Yellow
$toolchainFile = Join-Path $env:VCPKG_ROOT "scripts\buildsystems\vcpkg.cmake"
cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE="$toolchainFile"

if ($LASTEXITCODE -ne 0) {
    Write-Host "CMake configuration failed." -ForegroundColor Red
    exit 1
}

Write-Host "CMake configured successfully!" -ForegroundColor Green
Write-Host ""

# Build the project
Write-Host "Building project..." -ForegroundColor Yellow
cmake --build build --config Release

if ($LASTEXITCODE -ne 0) {
    Write-Host "Build failed." -ForegroundColor Red
    exit 1
}

Write-Host ""
Write-Host "=== Setup Complete! ===" -ForegroundColor Green
Write-Host ""
Write-Host "To run the server:" -ForegroundColor Cyan
Write-Host "  .\build\Release\GuessIOServer.exe" -ForegroundColor White
Write-Host ""

Pop-Location

Pop-Location
