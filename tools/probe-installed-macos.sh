#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
mkdir -p artifacts
OUT="$(mktemp -d "$PWD/artifacts/independent-XXXXXX")"
open -n "$PWD/artifacts/package/CrossShot.app" --args --probe "$OUT"
# 'open' returning zero does NOT mean the probe passed. Read the actual result.
python3 - "$OUT" <<'PY'
import sys,time,json,shutil
from pathlib import Path
out=Path(sys.argv[1]); report=out/'mac-probe.json'
for _ in range(140):
    if report.exists():break
    time.sleep(.25)
else:raise SystemExit('No probe report within 35 seconds')
data=json.loads(report.read_text())
stable=Path('artifacts/launchservices-probe');stable.mkdir(exist_ok=True)
shutil.copy2(report,stable/report.name)
print(json.dumps(data,ensure_ascii=False,indent=2))
if data['capture']['status']!='passed':
    print('Allow CrossShot screen recording in System Settings; quit/relaunch and rerun this probe.')
    raise SystemExit(3)
if not data['pin']['visualOcclusionVerified']:raise SystemExit(4)
PY
