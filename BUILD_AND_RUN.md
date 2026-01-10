# Quick Start: Build and Run

## Step 1: Install Dependencies (First Time Only)
```powershell
vcpkg install boost-asio grpc protobuf nlohmann-json --triplet x64-windows
```

## Step 2: Build the Project

### Option A: Using VSCode
1. Press `Ctrl+Shift+P`
2. Type "CMake: Configure" and press Enter
3. Press `F7` to build

### Option B: Using Terminal
```powershell
# Configure
cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE="$env:VCPKG_ROOT\scripts\buildsystems\vcpkg.cmake"

# Build
cmake --build build --config Debug
```

## Step 3: Run the Server

### Option A: Using VSCode
- Press `F5` to debug, or
- Press `Ctrl+Shift+P` → "CMake: Run Without Debugging"

### Option B: Using Terminal
```powershell
.\build\Debug\GuessIOServer.exe
```

## Optional: Create config.json
The server can use environment variables or a `config.json` file:
```json
{
    "TWITCH_OAUTH": "your_oauth_token",
    "TWITCH_NICK": "your_bot_username",
    "TWITCH_CHANNEL": "your_channel_name"
}
```
