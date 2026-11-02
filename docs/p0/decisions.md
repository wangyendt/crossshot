# P0 实施决策

1. **隔离 SDK**：保留系统 Qt 5，Qt6/ORT 放 `.deps/`；权重放 `models/`，均 Git 忽略。Qt6.8.3 在 Xcode27.2 缺 AGL SDK stub，切换官方 Qt6.12.0，而非修改系统 SDK。
2. **截图取物理模式尺寸**：`CGDisplayPixelsWide` 在实测缩放显示模式中返回1280，Qt逻辑1280且DPR2；改为 `CGDisplayModeGetPixelWidth/Height` 后得到2560×1440。四角锚点与DPR一起验收，不能只有“截图非空”。
3. **C++ CPU 本地 OCR**：为了先证明ARM部署/模型切换，选已校验的PP-OCRv4/v5 mobile转换权重；暂缓server包及复杂多边形后处理。明确没有完整官方精度等价结论。
4. **Worker不初始化GUI**：打包测试暴露QGuiApplication依赖未部署的offscreen插件；改为QCoreApplication（图像处理仍用QtGui的QImage）。同时修复仅在主程序重链时复制worker造成旧helper残留的CMake规则，复制任务每次构建检查更新。
5. **开发包而非发行包**：Qt/ORT动态库嵌入bundle，模型留在仓库；测试clean env的实际加载路径，ad-hoc签名。首次权限、另一台无SDK机器、公证、模型许可与完全可搬移安装包仍待验证。
6. **不伪装平台实现**：非macOS捕获返回not_implemented；CI暂时仅core。macshot算法移植保持在后续阶段。

7. **独立TCC身份必须单独测试**：clean-env清除环境变量不消除父进程权限归属。LaunchServices启动已发现独立App缺少授权；新增探针读取JSON而非相信open退出码，未授权保持blocked。
