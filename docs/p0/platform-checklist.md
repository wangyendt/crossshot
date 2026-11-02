# P0 后续平台验证清单

状态：除 evidence 中明确记录的本机 Mac ARM 项外，以下全部为 **未执行**。core-only CI 即使通过，也不能替代 UI/捕获/权限真机验证。

## Mac ARM 继续项

- [ ] 内建 Retina 屏、多个显示器、不同 DPR、负坐标、旋转、热插拔；目前只测 Q27P10 主屏。
- [ ] 新用户/独立 App 身份首次 TCC 授权、拒绝、撤销、重启；本轮已有屏幕捕获访问权，不能证明首次权限流程。
- [ ] 拖动贴图到其他 DPR 显示器后1:1缩放重算；现有实现只在设置缩放时读取 screen DPR。
- [ ] 多 Spaces、全屏应用、失焦后的浮窗；当前只测普通窗口上方置顶及透明度像素合成。
- [ ] 最低 macOS 14.4 真机、未装 SDK 的另一台机器、Gatekeeper、公证和正式签名；本轮只是当前机器的 clean-env 包验证。
- [ ] server 高精度 Paddle 包、官方 Python/Paddle 全流程 parity、旋转/小字/多列/长图；当前是水平探针。
- [ ] CPU 峰值/长时内存、连续100次操作、并发模型任务、真正崩溃后重启（取消 kill 已测，不等于所有崩溃路径）。
- [ ] 富文本 HTML/RTF、屏幕权限撤销中途捕获、HDR/SDR、权限提示时焦点、截图保存失败。

## Windows x86-64

| ID | 工作 | 必须留存的验收证据 |
|---|---|---|
| W01 | MSVC C++20、Qt6 x64、ORT x64 构建与部署 | 干净 Win11 非开发机安装运行，CPU 指令集/VC runtime 清单 |
| W02 | 实现 Windows.Graphics.Capture 原生适配 | display/window 选源、权限错误、resize、帧时钟与释放顺序 |
| W03 | DPI awareness + 100/125/150/200% 多屏 | 物理像素选择误差、负坐标、混合缩放/旋转与热插拔 |
| W04 | HWND Pin 策略 | 置顶、透明度、锁定、焦点与窗口交互；全屏应用行为 |
| W05 | 本地 OCR 两包 CPU parity | 同一合成与真实样本、字典一致、DLL 装载、cold/warm、内存 |
| W06 | worker 生命周期与权限 | kill/超时、缺失DLL、损坏模型、Unicode路径、只读目录 |
| W07 | CF_HTML/RTF/图片/文件剪贴板 | 多来源格式保留、退出后粘贴、MP4/GIF接收端 |
| W08 | 后续录屏探针 | WASAPI loopback+mic、设备切换、鼠标事件、音画同步、编码可分发性 |
| W09 | 桌面边界 | RDP、多显卡、锁屏、安全桌面、受保护画面；明确不支持而不是绕过 |

## Linux x86-64：必须拆开 X11 与 Wayland

| ID | 工作 | 必须留存的验收证据 |
|---|---|---|
| L01 | Ubuntu24.04 基线 Qt/ORT 构建、glibc与插件依赖 | 不依赖开发环境的包；字体、中英文字典、加载路径 |
| L02 | X11 原生捕获适配 | XShm/窗口捕获、不同WM、多屏坐标/缩放、帧率 |
| L03 | Wayland Portal + PipeWire 实现 | GNOME/KDE各自版本、选源/授权、关闭session、源变化、restore token策略 |
| L04 | Wayland Pin 能力验证 | 任意位置/置顶不保证；实际能力探测与UI降级，不将XWayland绕路当通用支持 |
| L05 | Qt浮窗/透明度/锁定 | X11与Wayland分别测；系统移动窗口和应用拖动锁定分开 |
| L06 | 快捷键/托盘 | X11注册冲突、GlobalShortcuts Portal 可用性；没有托盘时保留应用入口 |
| L07 | 两包 OCR CPU parity | x86-64 指令集、线程竞争、内存、库版本、字典与Mac结果对照 |
| L08 | MIME/selection 生命周期 | HTML/纯文本/图片/文件；后台剪贴板与进程退出后粘贴 |
| L09 | 后续录屏/音频/鼠标 | PipeWire/PulseAudio monitor+mic；ScreenCast不等于含系统音；cursor metadata不等于能读取全局点击 |
| L10 | 包和权限 | AppImage/发行版包；Flatpak单独验证Portal/资源/密钥存储；无需root |

## 全平台尚未执行

- hx470 /v1/models、/v1/text 活体调用和真实凭据验证；本轮无远程请求。
- ScrollFrameMatcher/ScrollStitcher C++ 移植、快滚曲线和 GapDetected；macshot 本轮没有代码移植。
- 音画、MP4/GIF、双音源、媒体剪贴板；这些仍属于总体 P0 剩余风险项。
- 完整依赖 SBOM、Qt/模型许可溯源、公开发布源码/构建对应性；本轮仅本地仓库与开发包。
