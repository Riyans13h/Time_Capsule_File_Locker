#!/bin/bash

# Time Capsule File Locker WebAssembly Build Script
echo "Building Time Capsule Core WebAssembly module..."

# Check if Emscripten is available
if ! command -v emcc &> /dev/null; then
    echo "Error: Emscripten not found. Please install and activate Emscripten."
    echo "See: https://emscripten.org/docs/getting_started/downloads.html"
    exit 1
fi

# Create build directory
mkdir -p build
cd build

# Build with Emscripten
emcc \
    -O3 \
    -s WASM=1 \
    -s NO_EXIT_RUNTIME=1 \
    -s EXTRA_EXPORTED_RUNTIME_METHODS='["ccall", "cwrap"]' \
    -s EXPORTED_FUNCTIONS='["_malloc", "_free"]' \
    -s ALLOW_MEMORY_GROWTH=1 \
    -s MODULARIZE=1 \
    -s EXPORT_ES6=1 \
    -s SINGLE_FILE=1 \
    -s ENVIRONMENT=web \
    --bind \
    ../../cpp_core/main.cpp \
    ../../cpp_core/utils.cpp \
    ../../cpp_core/compress.cpp \
    ../../cpp_core/decompress.cpp \
    ../../cpp_core/encrypt.cpp \
    ../../cpp_core/decrypt.cpp \
    ../../cpp_core/hash.cpp \
    ../../cpp_core/metadata.cpp \
    ../../cpp_core/time_lock.cpp \
    -I../../cpp_core \
    -o timecapsule_core.js

# Check if build was successful
if [ $? -eq 0 ]; then
    echo "Build successful! WebAssembly module created:"
    echo " - build/timecapsule_core.js"
    echo " - build/timecapsule_core.wasm"
    
    # Copy to web directory
    cp timecapsule_core.js ../../../web/
    cp timecapsule_core.wasm ../../../web/
    echo "Files copied to web directory"
else
    echo "Build failed!"
    exit 1
fi