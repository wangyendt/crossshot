# 本轮测试边界

沿用已确认计划中的公开边界：ScreenCapture（真实原生画面与物理坐标）、Pin（公开缩放/锁定/透明度及实际合成）、OCR provider（原生worker CLI/JSON）、GUI操作（按钮/模型选择器）、无Qt核心（裁剪与CTC）。

- core_tests：14个独立断言；首个坐标测试先失败再实现，red/green日志留存。未宣称整个工程严格逐项TDD。
- widget_tests：4个业务测试；Qt另有init/cleanup记录，不额外计作业务能力。
- ocr_contracts.py：8个公开CLI场景；测试程序是Python，识别执行始终是C++二进制。
- --probe：真正显示合成图→ScreenCaptureKit→裁剪→颜色锚点；绘制普通黑窗遮挡Pin，通过55%白色Pin的灰色合成像素验证窗口上方显示。
- --ui-smoke：真实QProcess worker，切换两模型、取消并恢复、换图后抑制旧结果；只使用自有测试图。
- package：重新运行worker/GUI/截图探针；验证签名、架构和实际动态库加载位置。

未测：丰富OCR质量数据、官方全流程parity、其他桌面、全屏Spaces、首次权限、实际网络服务。
