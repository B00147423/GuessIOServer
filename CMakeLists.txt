cmake_minimum_required(VERSION 3.20)
project(GuessIOServer)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find packages
find_package(Boost REQUIRED COMPONENTS system)
find_package(Protobuf REQUIRED)
find_package(gRPC REQUIRED)
find_package(nlohmann_json REQUIRED)

# Include directories
include_directories(${Boost_INCLUDE_DIRS})
include_directories(${Protobuf_INCLUDE_DIRS})
include_directories(${gRPC_INCLUDE_DIRS})
include_directories(src)
include_directories(src/libs)
include_directories(proto/proto_gen)

# Source files
set(SOURCES
    src/main.cpp
    src/server.cpp
    src/session.cpp
    src/room.cpp
    src/roomManager.cpp
    src/TwitchClient.cpp
    src/TwitchBotManager.cpp
    src/GameProtocol.cpp
    src/grpc_server.cpp
    src/libs/sha1.c
    proto/proto_gen/guessio.pb.cc
    proto/proto_gen/guessio.grpc.pb.cc
)

# Executable
add_executable(${PROJECT_NAME} ${SOURCES})

# Link libraries
target_link_libraries(${PROJECT_NAME}
    ${Boost_LIBRARIES}
    protobuf::libprotobuf
    gRPC::grpc++
    gRPC::grpc++_reflection
    nlohmann_json::nlohmann_json
)

# Windows-specific settings
if(WIN32)
    target_compile_definitions(${PROJECT_NAME} PRIVATE _WIN32_WINNT=0x0A00)
endif()
