# Project Setup Guide

This guide will help you set up the GuessIOServer project in VSCode.

## Prerequisites

1. **Visual Studio 2022** (or later) with C++ development tools
2. **CMake** (3.20 or later)
3. **vcpkg** - C++ package manager

## Step 1: Install vcpkg

If you don't have vcpkg installed:

1. Clone vcpkg:
   ```powershell
   git clone https://github.com/Microsoft/vcpkg.git C:\vcpkg
   ```

2. Run the bootstrap script:
   ```powershell
   cd C:\vcpkg
   .\bootstrap-vcpkg.bat
   ```

3. Set the environment variable:
   ```powershell
   [System.Environment]::SetEnvironmentVariable("VCPKG_ROOT", "C:\vcpkg", "User")
   ```
   
   Then restart VSCode for the environment variable to take effect.

## Step 2: Install Project Dependencies

Install all required libraries using vcpkg:

The project uses vcpkg manifest mode, so dependencies are installed automatically when you configure CMake. However, you can also install them manually:

```powershell
vcpkg install --triplet x64-windows
```

This will install:
- **boost-asio** - Asynchronous I/O library
- **boost-beast** - WebSocket support
- **grpc** - gRPC framework
- **protobuf** - Protocol Buffers
- **nlohmann-json** - JSON library

**Note:** This may take 15-30 minutes as it compiles from source.

## Step 3: Configure VSCode

1. Install the **CMake Tools** extension in VSCode
2. Install the **C/C++** extension in VSCode
3. Restart VSCode after setting the `VCPKG_ROOT` environment variable

## Step 4: Build the Project

### Option A: Using VSCode CMake Tools

1. Press `Ctrl+Shift+P` and select "CMake: Configure"
2. Select your compiler (Visual Studio 2022)
3. Press `F7` or use "CMake: Build" to build the project

### Option B: Using Command Line

```powershell
# Configure
cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE=$env:VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake

# Build
cmake --build build --config Release
```

## Step 5: Run the Server

After building, the executable will be in:
- `build/Release/GuessIOServer.exe` (Release)
- `build/Debug/GuessIOServer.exe` (Debug)

You can run it from the command line or set up a launch configuration in VSCode.

## Troubleshooting

### vcpkg not found
- Make sure `VCPKG_ROOT` environment variable is set
- Restart VSCode after setting the variable
- Check that the path points to your vcpkg installation

### CMake can't find packages
- Ensure you've installed all dependencies with vcpkg
- Verify the triplet matches (x64-windows)
- Check that `CMAKE_TOOLCHAIN_FILE` is correctly set

### Build errors
- Make sure you have Visual Studio 2022 with C++ tools installed
- Check that all source files are present
- Verify proto files are generated in `proto/proto_gen/`

## Additional Notes

- The project uses Protocol Buffers. The generated files are already in `proto/proto_gen/`
- If you need to regenerate proto files, you'll need `protoc` and `grpc_cpp_plugin` from vcpkg
- The project expects a `config.json` file in the root directory (optional, can use environment variables instead)
