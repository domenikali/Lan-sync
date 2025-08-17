cmake_minimum_required(VERSION 3.16)
project(LanSync)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find packages
find_package(Threads REQUIRED)
find_package(OpenSSL REQUIRED)

# Include directories
include_directories(
    include
    third_party
)

# Source files
file(GLOB SOURCES "src/*.cpp")

# Executable
add_executable(lan_sync_server ${SOURCES})

# Link libraries
target_link_libraries(lan_sync_server 
    Threads::Threads
    OpenSSL::SSL
    OpenSSL::Crypto
)

# Compiler options
target_compile_options(lan_sync_server PRIVATE 
    -Wall 
    -Wextra 
    -O3
)
