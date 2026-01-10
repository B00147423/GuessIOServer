# GuessIOServer

A C++ server application for a guessing game with Twitch integration, WebSocket support, and gRPC services.

## Quick Start

### Prerequisites
- **Windows 10/11**
- **Visual Studio 2022** (or later) with C++ development tools
- **CMake** 3.20 or later
- **Git**

### One-Command Setup (Recommended)

Run this in PowerShell from the project directory:

```powershell
.\setup.ps1
```

This script will:
1. Install vcpkg (if not already installed)
2. Install all dependencies
3. Configure and build the project

### Manual Setup

1. **Clone the repository:**
   ```powershell
   git clone <your-repo-url>
   cd GuessIOServer
   ```

2. **Install vcpkg** (if not already installed):
   ```powershell
   git clone https://github.com/Microsoft/vcpkg.git C:\vcpkg
   cd C:\vcpkg
   .\bootstrap-vcpkg.bat
   [System.Environment]::SetEnvironmentVariable("VCPKG_ROOT", "C:\vcpkg", "User")
   ```
   **Important:** Restart your terminal/VSCode after setting the environment variable.

3. **Build the project:**
   ```powershell
   cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE="$env:VCPKG_ROOT\scripts\buildsystems\vcpkg.cmake"
   cmake --build build --config Release
   ```

4. **Run the server:**
   ```powershell
   .\build\Release\GuessIOServer.exe
   ```

## Dependencies

The project uses vcpkg to manage dependencies:
- **boost-asio** - Asynchronous I/O
- **boost-beast** - WebSocket support
- **grpc** - gRPC framework
- **protobuf** - Protocol Buffers
- **nlohmann-json** - JSON parsing

All dependencies are automatically installed when you build the project (via `vcpkg.json`).

## Configuration

The server can be configured via:
1. **Environment variables:**
   - `TWITCH_OAUTH` - Twitch OAuth token
   - `TWITCH_NICK` - Bot username
   - `TWITCH_CHANNEL` - Channel name

2. **config.json** (optional):
   ```json
   {
       "TWITCH_OAUTH": "your_token",
       "TWITCH_NICK": "your_bot",
       "TWITCH_CHANNEL": "your_channel"
   }
   ```

## Ports

- **9001** - WebSocket server
- **50051** - gRPC server

## Building with VSCode (Using Buttons)

### Quick Setup:
1. Install **CMake Tools** extension (`Ctrl+Shift+X` → search "CMake Tools")
2. Install **C/C++** extension (if not already installed)
3. Reload VSCode

### Building with Buttons:
- **Status Bar (Bottom):** Look for build/configure buttons in the status bar
- **Keyboard:** Press `F7` to build, `Ctrl+Shift+P` → "CMake: Build"
- **Command Palette:** `Ctrl+Shift+P` → "Tasks: Run Task" → "CMake: Build"
- **Debug:** Press `F5` to build and debug

## Troubleshooting

**vcpkg not found:**
- Make sure `VCPKG_ROOT` environment variable is set
- Restart terminal/VSCode after setting it

**Build fails:**
- Ensure Visual Studio 2022 with C++ tools is installed
- Run `vcpkg install` manually if needed

**Can't find dependencies:**
- The project uses manifest mode - dependencies install automatically
- First build may take 15-30 minutes (compiling from source)

