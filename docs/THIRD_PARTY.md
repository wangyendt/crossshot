# Provenance and distribution boundary

- Application source: GPL-3.0; no macshot source has been translated in this P0. macshot's design/audit informed the plan only; future translated modules must retain original notices.
- Qt 6.12.0 qtbase: official Qt online archive through aqtinstall 3.3.0; dynamically linked. Audit Qt module and bundled third-party notices before public release. https://www.qt.io/licensing/open-source-lgpl-obligations
- ONNX Runtime 1.23.2: Microsoft official macOS arm64 release, SHA256 pinned in bootstrap. MIT plus its ThirdPartyNotices, copied into local package. https://github.com/microsoft/onnxruntime/releases/tag/v1.23.2
- PaddleOCR inference conventions (BGR preprocessing, normalization and CTC decoding) reviewed from PaddleOCR/RapidOCR Apache-2.0 sources. Our horizontal connected-component detector postprocessing is a P0 simplification, not a full implementation of their polygon/unclip pipeline.
- Referenced RapidOCR source: https://github.com/RapidAI/RapidOCR/tree/80b427d69bfc5b91a12913394875910296c6f315/python/rapidocr ; Apache notice included in docs/Apache-2.0-RapidOCR.txt.
- Model URLs and hashes: tools/models.lock.json, pinned ModelScope RapidAI/RapidOCR revision v3.10.0. Converted PaddleOCR weights are downloaded only by explicit provisioning; they are excluded from Git and the app bundle. Model redistribution/license provenance must be reviewed before a public binary/model release.
- This is a LOCAL developer package. Ad-hoc signing is neither notarization nor completion of public-distribution licensing requirements. No upload/publish has occurred.
