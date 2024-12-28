cmake_minimum_required(VERSION 3.20)
project(DistRM)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(FindPkgConfig)

# Find required packages
find_package(Protobuf CONFIG REQUIRED)
find_package(gRPC CONFIG REQUIRED)
pkg_check_modules(PROCPS REQUIRED libproc2)

# Create proto library
add_library(protolib)

# Set proto files
set(PROTO_FILES
    proto/system_monitor.proto
)

# Generate protobuf files
protobuf_generate(
    TARGET protolib
    LANGUAGE cpp
    PROTOS ${PROTO_FILES}
)

# Generate gRPC files
protobuf_generate(
    TARGET protolib
    LANGUAGE grpc
    GENERATE_EXTENSIONS .grpc.pb.h .grpc.pb.cc
    PLUGIN protoc-gen-grpc=$<TARGET_FILE:gRPC::grpc_cpp_plugin>
    PROTOS ${PROTO_FILES}
)

# Configure proto library
target_link_libraries(protolib
    PUBLIC
    protobuf::libprotobuf
    gRPC::grpc++
    gRPC::grpc++_reflection
)

target_include_directories(protolib
    PUBLIC
    ${CMAKE_CURRENT_BINARY_DIR}
)

# Create server executable
add_executable(server src/server.cpp 
    src/client.cpp
    src/server_security.hpp)
target_link_libraries(server
    PRIVATE
    protolib
)