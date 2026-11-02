#pragma once
#include <QImage>
#include <QJsonObject>
#include <QString>
#include <memory>
namespace crossshot {
class PaddleOcr {
    struct Impl;
    std::unique_ptr<Impl> impl_;

  public:
    explicit PaddleOcr(const QString &modelDirectory);
    ~PaddleOcr();
    QJsonObject recognize(const QImage &image);
};
} // namespace crossshot
