#include "fixture.hpp"
#include <QPainter>
namespace crossshot {
QImage makeFixture() {
    QImage im(1200, 420, QImage::Format_RGB32);
    im.fill(Qt::white);
    QPainter p(&im);
    p.fillRect(0, 0, 20, 20, Qt::red);
    p.fillRect(1180, 0, 20, 20, Qt::green);
    p.fillRect(0, 400, 20, 20, Qt::blue);
    p.fillRect(1180, 400, 20, 20, Qt::yellow);
    QFont f("PingFang SC");
    f.setPixelSize(40);
    p.setFont(f);
    p.setPen(Qt::black);
    p.drawText(55, 100, QString::fromUtf8("跨平台截图 OCR 风险验证"));
    p.drawText(55, 200, "CrossShot C++20 Qt 6");
    p.drawText(55, 300, "Invoice 12345.67 ABC xyz");
    return im;
}
} // namespace crossshot
