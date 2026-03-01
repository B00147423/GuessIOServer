# GuessIOServer

A C++ server application for a guessing game with Twitch integration, WebSocket support, and gRPC services.

## Quick Start (Docker - Recommended)

The server is containerized and runs automatically with Docker Compose. See the main project [README.md](../README.md) for full setup instructions.

```bash
# From project root
docker-compose up --build
```

The server will be available on:
- **Port 9001** - WebSocket server
- **Port 50051** - gRPC server

## Configuration

The server can be configured via environment variables in `docker-compose.yml`:
- `TWITCH_OAUTH` - Twitch OAuth token
- `TWITCH_NICK` - Bot username
- `TWITCH_CHANNEL` - Channel name

Or via `config.json` file (mounted as volume):
```json
{
    "TWITCH_OAUTH": "your_token",
    "TWITCH_NICK": "your_bot",
    "TWITCH_CHANNEL": "your_channel"
}
```

## Local Development (Optional)

If you need to build and run the server locally for development:

### Prerequisites
- **Windows 10/11** (or Linux)
- **Visual Studio 2022** (Windows) or **GCC/Clang** (Linux) with C++ development tools
- **CMake** 3.20 or later
- **vcpkg** - C++ package manager

### Quick Setup (Windows)

Run the automated setup script:
```powershell
.\setup.ps1
# or
.\setup.bat
```

This will:
1. Install vcpkg (if not already installed)
2. Install all dependencies
3. Configure and build the project

### Manual Setup

1. **Install vcpkg** (if not already installed):
   ```powershell
   git clone https://github.com/Microsoft/vcpkg.git C:\vcpkg
   cd C:\vcpkg
   .\bootstrap-vcpkg.bat
   [System.Environment]::SetEnvironmentVariable("VCPKG_ROOT", "C:\vcpkg", "User")
   ```
   **Important:** Restart your terminal/VSCode after setting the environment variable.

2. **Build the project:**
   ```powershell
   # Configure
   cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE="$env:VCPKG_ROOT\scripts\buildsystems\vcpkg.cmake"
   
   # Build
   cmake --build build --config Release
   ```

3. **Run the server:**
   ```powershell
   .\build\Release\GuessIOServer.exe
   ```

### Building with VSCode

1. Install **CMake Tools** extension (`Ctrl+Shift+X` → search "CMake Tools")
2. Install **C/C++** extension (if not already installed)
3. Press `Ctrl+Shift+P` → "CMake: Configure"
4. Press `F7` to build, or `F5` to debug

## Dependencies

The project uses vcpkg to manage dependencies:
- **boost-asio** - Asynchronous I/O
- **boost-beast** - WebSocket support
- **grpc** - gRPC framework
- **protobuf** - Protocol Buffers
- **nlohmann-json** - JSON parsing

All dependencies are automatically installed when you build the project (via `vcpkg.json`).

## Troubleshooting

**vcpkg not found:**
- Make sure `VCPKG_ROOT` environment variable is set
- Restart terminal/VSCode after setting it

**Build fails:**
- Ensure Visual Studio 2022 with C++ tools is installed (Windows)
- Run `vcpkg install` manually if needed

**Can't find dependencies:**
- The project uses manifest mode - dependencies install automatically
- First build may take 15-30 minutes (compiling from source)

**CMake can't find packages:**
- Verify the triplet matches (x64-windows on Windows, x64-linux on Linux)
- Check that `CMAKE_TOOLCHAIN_FILE` is correctly set

**Proto files not generated:**
- Proto files are auto-generated during build
- If issues occur, ensure `protoc` and `grpc_cpp_plugin` are available from vcpkg
