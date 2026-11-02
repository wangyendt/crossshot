#include "capture.hpp"
namespace crossshot {
bool screenPermission() {
    return false;
}
bool requestScreenPermission() {
    return false;
}
void capturePrimary(std::function<void(CaptureResult)> cb) {
    cb({{}, "native_capture_not_implemented_on_this_platform", 0});
}
void configureNativePin(QWidget *) {}
QJsonObject nativePinInfo(QWidget *) {
    return {{"status", "not_implemented"}};
}
} // namespace crossshot
