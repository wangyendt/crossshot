#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo \
 -DCMAKE_PREFIX_PATH="$PWD/.deps/qt/6.12.0/macos" \
 -DONNXRUNTIME_ROOT="$PWD/.deps/onnxruntime" \
 -DCMAKE_OSX_ARCHITECTURES=arm64 -DCMAKE_OSX_DEPLOYMENT_TARGET=14.4
cmake --build build --parallel 6
ctest --test-dir build --output-on-failure
