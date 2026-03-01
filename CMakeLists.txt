cmake_minimum_required(VERSION 3.20)
project(GuessIOServer)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find packages
find_package(Boost REQUIRED COMPONENTS system)
find_package(Protobuf CONFIG REQUIRED)
find_package(gRPC CONFIG REQUIRED)
find_package(nlohmann_json REQUIRED)

# Proto file
set(PROTO_FILE ${CMAKE_CURRENT_SOURCE_DIR}/proto/guessio.proto)
set(PROTO_OUT_DIR ${CMAKE_CURRENT_BINARY_DIR}/proto_gen)

file(MAKE_DIRECTORY ${PROTO_OUT_DIR})

add_library(proto
    ${PROTO_OUT_DIR}/guessio.pb.cc
    ${PROTO_OUT_DIR}/guessio.grpc.pb.cc
)

add_custom_command(
    OUTPUT
        ${PROTO_OUT_DIR}/guessio.pb.cc
        ${PROTO_OUT_DIR}/guessio.pb.h
        ${PROTO_OUT_DIR}/guessio.grpc.pb.cc
        ${PROTO_OUT_DIR}/guessio.grpc.pb.h
    COMMAND $<TARGET_FILE:protobuf::protoc>
    ARGS
        --cpp_out=${PROTO_OUT_DIR}
        --grpc_out=${PROTO_OUT_DIR}
        -I ${CMAKE_CURRENT_SOURCE_DIR}/proto
        --plugin=protoc-gen-grpc=$<TARGET_FILE:gRPC::grpc_cpp_plugin>
        ${PROTO_FILE}
    DEPENDS ${PROTO_FILE}
    VERBATIM
)

target_link_libraries(proto
    PUBLIC
    protobuf::libprotobuf
    gRPC::grpc++
)

target_include_directories(proto
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/proto
    ${PROTO_OUT_DIR}
)

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
)

add_executable(${PROJECT_NAME} ${SOURCES})

# Make proto include directory available to executable
target_include_directories(${PROJECT_NAME} PRIVATE ${PROTO_OUT_DIR})

target_link_libraries(${PROJECT_NAME}
    PRIVATE
    proto
    ${Boost_LIBRARIES}
    gRPC::grpc++_reflection
    nlohmann_json::nlohmann_json
)

if(WIN32)
    target_compile_definitions(${PROJECT_NAME} PRIVATE _WIN32_WINNT=0x0A00)
endif()