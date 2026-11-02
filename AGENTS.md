# CrossShot development

Read README.md, CONTEXT.md, docs/p0/results.md before changes. Keep core independent of Qt/platform headers.
Run tools/build-macos.sh and tests/ocr_contracts.py for Mac changes; use core-only preset elsewhere.
Never commit .deps, models, arbitrary user screenshots, tokens, or build artifacts.
Synthetic fixture evidence is allowed. Never claim Windows/Linux UI or native backends are verified by core-only CI.
Do not silently replace native capture with synthetic/Qt grab output. Record permission denial as blocked.
OCR is local/offline by default. No hx470 calls until the remote phase is explicitly resumed.
