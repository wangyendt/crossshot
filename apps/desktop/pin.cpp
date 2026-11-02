#include "pin.hpp"
#include <QApplication>
#include <QClipboard>
#include <QContextMenuEvent>
#include <QMenu>
#include <QMouseEvent>
#include <QPainter>
#include <QScreen>
#include <QWheelEvent>
#include <cmath>
namespace crossshot {
PinWindow::PinWindow(QImage image, QWidget *parent)
    : QWidget(parent, Qt::Tool | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint),
      image_(std::move(image)) {
    setAttribute(Qt::WA_DeleteOnClose);
    setWindowTitle("CrossShot · 贴图（右键菜单）");
    // 100% means one source pixel per physical display pixel.
    double dpr = screen() ? screen()->devicePixelRatio() : 1;
    double fit = std::min(1.0, 900.0 * dpr / image_.width());
    setZoom(fit);
}
void PinWindow::setZoom(double z) {
    state_.setScale(z);
    double dpr = screen() ? screen()->devicePixelRatio() : 1;
    resize(std::max(1, static_cast<int>(image_.width() * state_.scale() / dpr)),
           std::max(1, static_cast<int>(image_.height() * state_.scale() / dpr)));
    update();
}
void PinWindow::setPinOpacity(double v) {
    state_.setOpacity(v);
    setWindowOpacity(state_.opacity());
}
void PinWindow::paintEvent(QPaintEvent *) {
    QPainter p(this);
    p.setRenderHint(QPainter::SmoothPixmapTransform);
    p.drawImage(rect(), image_);
    p.setPen(QColor("#3584e4"));
    p.drawRect(rect().adjusted(0, 0, -1, -1));
}
void PinWindow::mousePressEvent(QMouseEvent *e) {
    if (e->button() == Qt::LeftButton && !locked()) {
        dragging_ = true;
        dragOffset_ = e->globalPosition().toPoint() - pos();
    }
}
void PinWindow::mouseMoveEvent(QMouseEvent *e) {
    if (dragging_ && !locked())
        move(e->globalPosition().toPoint() - dragOffset_);
}
void PinWindow::mouseReleaseEvent(QMouseEvent *) {
    dragging_ = false;
}
void PinWindow::wheelEvent(QWheelEvent *e) {
    if (e->modifiers() & Qt::ControlModifier) {
        setPinOpacity(pinOpacity() + (e->angleDelta().y() > 0 ? .1 : -.1));
    } else {
        auto anchor = e->position();
        double old = zoom();
        auto origin = pos();
        setZoom(old * std::pow(1.1, e->angleDelta().y() / 120.0));
        if (!locked())
            move(origin + QPoint(qRound(anchor.x() * (1 - zoom() / old)),
                                 qRound(anchor.y() * (1 - zoom() / old))));
    }
    e->accept();
}
void PinWindow::contextMenuEvent(QContextMenuEvent *e) {
    QMenu m;
    m.addAction("复制图片", this, [this] { QApplication::clipboard()->setImage(image_); });
    auto lock = m.addAction("锁定位置");
    lock->setCheckable(true);
    lock->setChecked(locked());
    connect(lock, &QAction::toggled, this, &PinWindow::setLocked);
    m.addAction("实际像素 1:1", this, [this] { setZoom(1); });
    m.addAction("透明度 50%", this, [this] { setPinOpacity(.5); });
    m.addAction("透明度 100%", this, [this] { setPinOpacity(1); });
    m.addSeparator();
    m.addAction("关闭", this, &QWidget::close);
    m.exec(e->globalPos());
}
} // namespace crossshot
