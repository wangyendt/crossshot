# 工程目录调整（2026-11-02）

当前路径：`/Users/wayne/Documents/work/code/cpp/crossshot`。

选择 cpp 子目录是为了沿用 work/code 按语言分类的组织方式；本项目以 C++20/Qt 为主，Python 仅用于依赖下载和测试。

- 整体迁移了 .git、源代码、模型和隔离 SDK，保留全部提交历史；未创建远端或推送。
- 更新 README、P0 报告和报告生成脚本中的路径。
- 原构建缓存移入忽略目录 artifacts/build-before-relocation、artifacts/build-core-before-relocation；重新生成新路径构建树。
- Python bootstrap venv 重新创建；旧副本保留于 .deps/bootstrap-venv-before-relocation，不再用于正常启动。
- Qt6 qtpaths 返回新目录；重新构建通过，2个 CTest 套件和8个OCR CLI场景通过。
- 重新打包、签名检查和清空开发环境后的包内OCR通过。
- 已在 GitHub Desktop 的 Add Local Repository 中添加新路径，Current Repository 为 crossshot，分支 main；没有点击 Publish repository。

迁移不代表屏幕录制权限已获批准；独立启动的 TCC 授权仍由用户在系统设置中确认。此次未改动权限，也未重跑屏幕捕获验证。
