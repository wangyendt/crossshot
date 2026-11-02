#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
mkdir -p artifacts/latest
ctest --test-dir build --output-on-failure
set +e
build/CrossShot.app/Contents/MacOS/CrossShot --probe artifacts/latest
CAPTURE_STATUS=$?
set -e
build/CrossShot.app/Contents/MacOS/CrossShot --ui-smoke artifacts/latest
for model in ppocr-v4-mobile ppocr-v5-mobile; do
 build/crossshot-ocr --models "models/$model" --image artifacts/latest/fixture.png --repeat 3 --out "artifacts/latest/ocr-$model.json"
done
python3 tests/ocr_contracts.py
if [[ $CAPTURE_STATUS -ne 0 ]]; then
 echo "Native capture not passed (exit $CAPTURE_STATUS); inspect mac-probe.json. OCR success does not override this."
 exit "$CAPTURE_STATUS"
fi
