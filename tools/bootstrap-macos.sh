#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
[[ "$(uname -m)" == arm64 && "$(uname -s)" == Darwin ]] || { echo 'This bootstrap is Mac ARM only'; exit 1; }
PYTHON="${PYTHON:-python3}"
command -v cmake >/dev/null; command -v ninja >/dev/null
mkdir -p .deps
[[ -x .deps/bootstrap-venv/bin/python ]] || "$PYTHON" -m venv .deps/bootstrap-venv
.deps/bootstrap-venv/bin/python -m pip install 'aqtinstall==3.3.0'
if [[ ! -x .deps/qt/6.12.0/macos/bin/qt-cmake ]]; then
 .deps/bootstrap-venv/bin/aqt install-qt mac desktop 6.12.0 clang_64 --archives qtbase -O .deps/qt
fi
if [[ ! -f .deps/onnxruntime/lib/libonnxruntime.dylib ]]; then
 curl --fail --location --retry 2 --connect-timeout 20 --max-time 300 https://github.com/microsoft/onnxruntime/releases/download/v1.23.2/onnxruntime-osx-arm64-1.23.2.tgz -o .deps/onnxruntime.tgz
 echo 'b4d513ab2b26f088c66891dbbc1408166708773d7cc4163de7bdca0e9bbb7856  .deps/onnxruntime.tgz' | shasum -a 256 -c -
 tar -xzf .deps/onnxruntime.tgz -C .deps
 mv .deps/onnxruntime-osx-arm64-1.23.2 .deps/onnxruntime
fi
"$PYTHON" tools/fetch_models.py
