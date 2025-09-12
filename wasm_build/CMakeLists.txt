cmake_minimum_required(VERSION 3.15)
project(TimeCapsuleCore)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Set Emscripten specific flags
set(CMAKE_EXECUTABLE_SUFFIX ".html")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -s WASM=1 -s NO_EXIT_RUNTIME=1 -s EXTRA_EXPORTED_RUNTIME_METHODS=['ccall','cwrap']")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -s WASM=1 -s NO_EXIT_RUNTIME=1 -s EXTRA_EXPORTED_RUNTIME_METHODS=['ccall','cwrap']")

# Add source files
add_executable(timecapsule_core
    ../cpp_core/main.cpp
    ../cpp_core/utils.cpp
    ../cpp_core/compress.cpp
    ../cpp_core/decompress.cpp
    ../cpp_core/encrypt.cpp
    ../cpp_core/decrypt.cpp
    ../cpp_core/hash.cpp
    ../cpp_core/metadata.cpp
    ../cpp_core/time_lock.cpp
)

# Include directories
target_include_directories(timecapsule_core PRIVATE ../cpp_core)

# Link against Crypto++ (would need to be compiled to WASM first)
# For now, we'll use the built-in implementations