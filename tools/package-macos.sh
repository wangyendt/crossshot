#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
mkdir -p artifacts/package
APP="$PWD/artifacts/package/CrossShot.app"
ditto build/CrossShot.app "$APP"
cp build/crossshot-ocr "$APP/Contents/MacOS/crossshot-ocr"
.deps/qt/6.12.0/macos/bin/macdeployqt "$APP" \
 -executable="$APP/Contents/MacOS/crossshot-ocr" -always-overwrite
mkdir -p "$APP/Contents/Resources/licenses"
cp LICENSE "$APP/Contents/Resources/licenses/CrossShot-GPL-3.0.txt"
cp .deps/onnxruntime/LICENSE "$APP/Contents/Resources/licenses/ONNX-Runtime-MIT.txt"
cp .deps/onnxruntime/ThirdPartyNotices.txt "$APP/Contents/Resources/licenses/ONNX-Runtime-ThirdPartyNotices.txt"
cp docs/Apache-2.0-RapidOCR.txt "$APP/Contents/Resources/licenses/Apache-2.0.txt"
# Local developer package only. Not notarization or a complete public-distribution license bundle.
codesign --force --deep --sign - "$APP"
codesign --verify --deep --strict "$APP"
echo "$APP"
