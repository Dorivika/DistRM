cmake_minimum_required(VERSION 3.10)
project(system_monitor)

# Setup logging
set(LOG_DIR "${CMAKE_BINARY_DIR}/logs")
file(MAKE_DIRECTORY ${LOG_DIR})

set(CMAKE_CXX_COMPILER g++)

# Compiler flags
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -fdiagnostics-color=always")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Werror")

# Enable core dumps
execute_process(COMMAND bash -c "ulimit -c unlimited")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -rdynamic")

# Add build logging wrapper
add_custom_target(build_with_logs
    COMMAND ${CMAKE_COMMAND} --build . 2>&1 | tee ${LOG_DIR}/build.log
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "Building with logging..."
)
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