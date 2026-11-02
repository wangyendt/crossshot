#pragma once
#include "core.hpp"
#include <QImage>
#include <QWidget>
namespace crossshot {
class PinWindow : public QWidget {
    QImage image_;
    PinState state_;
    QPoint dragOffset_;
    bool dragging_{};

  public:
    explicit PinWindow(QImage image, QWidget *parent = nullptr);
    void setZoom(double);
    double zoom() const {
        return state_.scale();
    }
    void setPinOpacity(double);
    double pinOpacity() const {
        return state_.opacity();
    }
    void setLocked(bool locked) {
        state_.setLocked(locked);
    }
    bool locked() const {
        return state_.locked();
    }
    const QImage &original() const {
        return image_;
    }

  protected:
    void paintEvent(QPaintEvent *) override;
    void mousePressEvent(QMouseEvent *) override;
    void mouseMoveEvent(QMouseEvent *) override;
    void mouseReleaseEvent(QMouseEvent *) override;
    void wheelEvent(QWheelEvent *) override;
    void contextMenuEvent(QContextMenuEvent *) override;
};
} // namespace crossshot
