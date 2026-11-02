# CrossShot 术语

**Capture Asset**：不可变原图；P0 仅在内存和用户明确保存的位置存在。
**Pin**：图片浮窗；缩放、透明度与位置锁定是独立状态。P0 不支持富文本或持久化。
**物理像素**：实际捕获缓冲区像素，不等于 macOS 点坐标或 CGDisplayPixelsWide 返回值。
**Model Pack**：固定 detector + recognizer + 嵌入字典 + SHA256 manifest；P0 两包通过选择器切换。
**Local Worker**：C++ + ONNX Runtime CPU 子进程，无网络下载/远程 OCR。
**Passed**：仅针对记录的机器、显示器、输入与断言；不能推广为三端全功能通过。
