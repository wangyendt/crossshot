# Mac ARM P0 实测结果

日期：2026-11-02。仓库：`/Users/wayne/Documents/work/code/crossshot`。

## 结论

本轮要求的 **C++20/Qt6工程骨架、当前Mac截图、图片贴图、本地OCR部署/切换风险验证** 已完成并留存证据；**不表示完整P0或跨平台产品全部完成**。Windows/Linux原生后端、其他显示器/权限场景、完整OCR质量、富文本、长截图/录屏和hx470活体接入仍待验证。

## 环境

Apple M1 Max / arm64；macOS26.6；Xcode27.2 AppleClang21；Qt6.12.0；ONNX Runtime1.23.2 CPU；CMake3.30.2。当前主屏Q27P10，逻辑1280×720，DPR2，实际捕获2560×1440。部署目标14.4只是构建配置，未在14.4真机测试。

Qt/ORT使用仓库内隔离SDK，没有替换原有Qt5/Homebrew。模型包权重总计约36MiB，Git忽略；开发app约71MiB（不含模型）。SDK与下载缓存约1.3GiB，包含用于复现兼容性失败的旧Qt6.8.3。

## 截图与贴图

- 原生ScreenCaptureKit/SCScreenshotManager成功，不是Qt抓取自有widget冒充桌面截图。
- 完整采集帧2560×1440；裁剪自有测试窗口得到1200×420；四角RGB锚点和DPR断言通过。
- 本次采集回调耗时约 **214.5ms**（单次探针，不是p95）。没有将整个桌面落盘，只保留自有合成图区域。
- Pin原生NSWindow level3，高于普通窗口；锁定拖动与缩放几何通过。
- 透明度0.55：白色Pin覆盖黑色普通窗口，实际捕获RGB为140/140/140，验证了**真实合成与遮挡关系**，不是只读Qt属性。
- 未测多Spaces/全屏、跨DPR移动、权限首次授权/撤销。当前进程已有屏幕捕获访问权。

## 本地OCR：真正的C++推理

使用两套校验SHA256的PaddleOCR转换权重：PP-OCRv4 mobile与PP-OCRv5 mobile。Qt图片预处理→ONNX CPU检测→水平文本区域→ONNX识别→CTC解码。没有调用Python推理，也没有请求hx470或其他远程服务。

以下输入都是同一张从真实屏幕采集后裁剪的1200×420自有测试图，每模型同进程运行3次。warm仅后2次；模型加载与推理分开计时，**不能当作每次GUI点击的总耗时，也不是大样本性能基准**。

| 模型包 | 加载ms | 首次推理ms | warm推理ms | 进程峰值RSS MiB | 严格字符编辑数 |
|---|---:|---:|---:|---:|---:|
| ppocr-v4-mobile | 181.4 | 139.8 | 132.0–141.0 | 180.8 | 2 |
| ppocr-v5-mobile | 227.6 | 166.1 | 155.2–190.8 | 201.4 | 0 |

真值包含中文、英文、C++符号、大小写、数字、小数点和空格，共60字符（含换行）。v4遗漏中文段两侧的2个空格；v5在这个样例逐字匹配。**单图结果不代表普遍准确率，也不能据此宣布v5达到飞书水平。** 原始返回及框坐标在evidence/ocr-v4.json、ocr-v5.json。

当前后处理是水平连通域+矩形扩张，不包含完整DB多边形、方向分类和透视矫正；暂未证明与官方Paddle完整管线数值/精度等价。server高精度模型未测试。

## 自动化与打包

- 2个CTest套件通过：核心14个断言；Qt贴图4个业务用例（不将init/cleanup算能力）。
- 8个worker CLI契约通过：参数缺失、非法repeat、无效图片、缺manifest、损坏模型、路径穿越及两模型正常识别。
- GUI实测：两模型切换、子进程取消后恢复、换图后抑制旧任务结果。
- macdeployqt打包Qt/ORT，ad-hoc签名verify通过；清空开发环境变量后，包内GUI和worker均运行成功。
- dyld实际加载记录中没有`.deps`库路径；并非只在开发树运行成功。包内原生截图/Pin探针也通过。
- 模型仍从仓库models/读取，未打包；未公证、未在另一台无SDK机器验证，不是可公开分发的独立安装包。

## 本轮发现并解决的风险

1. Qt6.8.3 + 当前Xcode27.2：链接旧AGL失败。切换官方Qt6.12.0；失败日志保留，不修改系统SDK。
2. CGDisplayPixelsWide返回1280而非DPR2物理2560；改为显示模式pixelWidth/pixelHeight并增加DPR断言，防止低分辨率截图被算作通过。
3. OCR worker初始化QGuiApplication导致部署包缺offscreen插件而崩溃；改为QCoreApplication，从根本上去除worker的窗口系统初始化依赖。
4. CMake仅在主程序重链时复制helper导致包内旧worker残留；改为每次构建检查复制，并在打包时显式覆盖，重新验证GUI/worker。

## 后续

详细清单见docs/p0/platform-checklist.md。优先完成Mac多屏/权限边界与完整OCR后处理，再准备Windows与Linux真机探针。core-only CI已编写但没有远端仓库，**尚未运行Windows/Linux CI**。macshot本轮未移植任何业务算法；当前骨架为新实现。

## 复现

```bash
cd /Users/wayne/Documents/work/code/crossshot
bash tools/build-macos.sh
bash tools/run-probes-macos.sh
bash tools/package-macos.sh
bash tools/run-macos.sh
```

识别/截图输出含用户内容时默认留在本地，不应提交。evidence目录仅保留合成输入和脱敏日志。公开分发前补齐Qt/模型许可证清单、模型分发策略、签名/公证与最低OS验证。
