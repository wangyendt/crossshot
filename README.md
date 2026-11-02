# CrossShot — C++20 + Qt 6 P0

开源跨平台截图工具的**风险验证工程**，不是完成版。当前只验证 Mac ARM；Windows/Linux 原生捕获适配器明确返回 `not_implemented`，不以模拟画面冒充成功。

## 已实现

- CMake + C++20：无 Qt 核心、Qt Widgets UI、macOS Objective-C++ 适配器、独立 C++ OCR worker。
- ScreenCaptureKit 主屏截图；冻结画面后矩形选区；PNG 保存。
- 图片贴图：拖动、滚轮缩放、Ctrl+滚轮透明度、右键锁定/复制/关闭、恢复全部贴图。
- 本地 PaddleOCR v4/v5 mobile ONNX 模型切换；检测→水平区域→CTC 识别；独立子进程、取消、超时、过时结果隔离。
- 合成图原生截图/贴图合成探针、GUI 模型切换/取消/旧结果隔离测试、模型损坏与协议负例。
- 本机开发用 `.app` 包含 Qt 与 ONNX Runtime，ad-hoc 签名；**模型仍在仓库 models/，尚非可任意搬走的完整产品包**。

## 本机快速运行

```bash
cd /Users/wayne/Documents/work/code/cpp/crossshot
bash tools/run-macos.sh
```

**权限注意：开发进程截图已通过；正常双击独立启动目前缺少CrossShot自己的屏幕录制授权。系统权限申请已触发，请允许后完全退出/重新启动，再运行 `bash tools/probe-installed-macos.sh` 验证。**

应用会显示一张自有测试图。选择 `ppocr-v4-mobile` 或 `ppocr-v5-mobile` 后点“本地识别”。点“贴图”验证浮窗；“截图主屏幕”进入冻结画面的选区。尚无全局快捷键/托盘/标注/富文本。

## 从源码复现（Mac ARM）

需要 Xcode、CMake、Ninja、Python 3（只用于下载/测试）。所有 Qt/ORT SDK 安装在 `.deps`，不会替换系统 Qt 5。

```bash
bash tools/bootstrap-macos.sh  # 显式联网：Qt SDK、ORT SDK、两套模型
bash tools/build-macos.sh      # 原生 arm64 编译 + CTest
bash tools/run-probes-macos.sh # 会短暂显示合成窗口；只保存合成图区域
bash tools/package-macos.sh   # 本地开发包，非公证发布版
```

产品 OCR worker 是 C++ 程序，不依赖 Python/conda。运行时不下载模型、不调用 hx470，不上传图片。

固定开发版本：Qt 6.12.0、ONNX Runtime 1.23.2、aqtinstall 3.3.0。Mac 部署目标14.4，但只在当前26.6机器验证；最低系统版本尚待真机确认。模型来源、哈希和许可边界见 `tools/models.lock.json`、`docs/THIRD_PARTY.md`。

## 无 GUI 核心构建（为其他平台准备）

```bash
cmake -S . -B build-core -DCROSSSHOT_BUILD_GUI=OFF
cmake --build build-core --config Debug
ctest --test-dir build-core -C Debug --output-on-failure
```

Windows/Linux 的 GUI 编译可指定 `CMAKE_PREFIX_PATH` 与 `ONNXRUNTIME_ROOT`；**尚未实测**。如果只验证 Qt UI 可关闭 `CROSSSHOT_BUILD_OCR`。已提供三 OS 的 core-only CI workflow，但没有远程仓库，所以未触发任何 CI。

## 结构

- `libs/core/`：像素裁剪、贴图状态、CTC 解码；不依赖 Qt。
- `platform/macos/`：ScreenCaptureKit、物理像素尺寸、原生浮窗策略；`platform/stub.cpp` 明确拒绝未实现平台。
- `apps/desktop/`：P0 UI、图片 Pin、自有测试图、选区与探针入口。
- `providers/paddle/`：ONNX Runtime CPU 管线和命令行 worker。
- `tests/`：核心、Qt 贴图和 worker CLI 行为测试。
- `tools/`：隔离依赖、模型校验、编译、探针与打包脚本。
- `evidence/`：只含本次合成输入的可提交结果；`artifacts/` 是忽略的临时运行输出。
- `docs/p0/results.md`：实测与缺陷修复；`docs/p0/platform-checklist.md`：后续验收清单。

## 尚未完成 / 不应误解

1. 只验证当前外接显示器；混合 DPR、多屏跨屏、Spaces/全屏应用、首次授权/撤销需继续测试。
2. OCR DB 后处理是**水平连通域+矩形扩张的探针实现**，不是完整多边形/旋转/透视处理；检测长边960，输入≤32MP，区域≤256，识别行宽≤4096。只有小型合成样本，不代表飞书级准确率。
3. 当前比较的是 v4/v5 **两个 mobile 包**，不是轻量与 server 高精度包；server、官方完整管线对齐、模型勾选/安装 UI、置信度校准待做。
4. 当前 GUI 每次 OCR 启动一个 worker，首次加载成本可见；`--repeat` 用于同一进程内 warm 探针。常驻 worker/LRU 是后续优化，不拿 warm 数据冒充每次用户点击的总延时。
5. Pin 不支持富文本/持久化/点击穿透；长截图、音频、录屏、hx470 接口尚未实现。
6. `.app` 当前为本地开发包，未公证；模型未打包，公共分发依赖许可清单与最低OS验收尚未完成。

采用 GPL-3.0。没有移植 macshot 业务代码；未来算法翻译另行保留其版权/来源记录。未创建远程仓库、未推送任何内容。
