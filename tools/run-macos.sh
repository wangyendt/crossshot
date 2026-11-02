#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
APP="$PWD/artifacts/package/CrossShot.app"
[[ -d "$APP" ]] || APP="$PWD/build/CrossShot.app"
# LaunchServices launch is preferable to a shell for normal permission behavior.
open "$APP"
