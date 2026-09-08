#!/usr/bin/env bash
# Configures and builds schedsim with CMake + MinGW g++, reusing the
# vcpkg instance at C:/vcpkg (nlohmann-json, Catch2) set up for the other
# C++ projects in this workspace.
set -euo pipefail

cd "$(dirname "$0")"

VCPKG_ROOT="${VCPKG_ROOT:-C:/vcpkg}"

cmake -S . -B build \
    -G "MinGW Makefiles" \
    -DCMAKE_TOOLCHAIN_FILE="$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake" \
    -DVCPKG_TARGET_TRIPLET=x64-mingw-static \
    -DVCPKG_HOST_TRIPLET=x64-mingw-static \
    -DCMAKE_BUILD_TYPE=Release

cmake --build build --parallel

echo ""
echo "Built: build/schedsim.exe"
echo "Test binary: build/schedsim_tests.exe"
