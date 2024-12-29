cmake_minimum_required(VERSION 3.10)
project(system_monitor)

# Find required packages
find_package(Protobuf REQUIRED)
find_package(gRPC REQUIRED)
find_package(Threads REQUIRED)
find_package(PkgConfig REQUIRED)
find_package(absl REQUIRED)  # Add abseil package

# Find libproc2
pkg_check_modules(LIBPROC2 REQUIRED libproc2)

# Proto files
set(PROTO_FILES
    proto/system_monitor.proto
)

# Generate protobuf library
add_library(protolib)
target_include_directories(protolib PUBLIC 
    ${CMAKE_CURRENT_BINARY_DIR}
    ${LIBPROC2_INCLUDE_DIRS}
)

protobuf_generate(
    TARGET protolib
    LANGUAGE cpp
    PROTOS ${PROTO_FILES}
)

protobuf_generate(
    TARGET protolib
    LANGUAGE grpc
    GENERATE_EXTENSIONS .grpc.pb.h .grpc.pb.cc
    PLUGIN protoc-gen-grpc=$<TARGET_FILE:gRPC::grpc_cpp_plugin>
    PROTOS ${PROTO_FILES}
)

# Link protolib dependencies
target_link_libraries(protolib
    PUBLIC
    protobuf::libprotobuf
    gRPC::grpc++
    gRPC::grpc++_reflection
    absl::base
    absl::log
    absl::strings
)

# Server executable
add_executable(server src/server.cpp)
target_link_libraries(server
    PRIVATE
    protolib
    Threads::Threads
    ${LIBPROC2_LIBRARIES}
    absl::base
    absl::log
    absl::strings
)

target_include_directories(server PRIVATE ${LIBPROC2_INCLUDE_DIRS})
target_link_directories(server PRIVATE ${LIBPROC2_LIBRARY_DIRS})