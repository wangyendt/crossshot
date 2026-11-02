#pragma once
#include <QImage>
#include <QJsonObject>
#include <QString>
#include <functional>
class QWidget;
namespace crossshot {
struct CaptureResult {
    QImage image;
    QString error;
    double elapsedMs{};
};
bool screenPermission();
bool requestScreenPermission();
void capturePrimary(std::function<void(CaptureResult)> callback);
void configureNativePin(QWidget *);
QJsonObject nativePinInfo(QWidget *);
} // namespace crossshot
