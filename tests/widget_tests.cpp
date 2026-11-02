#include "fixture.hpp"
#include "pin.hpp"
#include <QApplication>
#include <QtTest>
using namespace crossshot;
class PinTests : public QObject {
    Q_OBJECT
  private slots:
    void originalPreserved() {
        PinWindow p(makeFixture());
        auto before = p.original();
        p.setZoom(.5);
        p.setPinOpacity(.5);
        QCOMPARE(p.original(), before);
    }
    void independentProperties() {
        PinWindow p(makeFixture());
        p.setLocked(true);
        p.setZoom(2);
        p.setPinOpacity(.6);
        QVERIFY(p.locked());
        QCOMPARE(p.zoom(), 2.0);
        QCOMPARE(p.pinOpacity(), .6);
    }
    void lockedDragDoesNotMove() {
        PinWindow p(makeFixture());
        p.move(100, 100);
        p.show();
        p.setLocked(true);
        QTest::mousePress(&p, Qt::LeftButton, {}, QPoint(10, 10));
        QTest::mouseMove(&p, QPoint(100, 100));
        QTest::mouseRelease(&p, Qt::LeftButton, {}, QPoint(100, 100));
        QCOMPARE(p.pos(), QPoint(100, 100));
    }
    void zoomResizes() {
        PinWindow p(makeFixture());
        p.setZoom(1);
        auto before = p.size();
        p.setZoom(.5);
        QCOMPARE(p.width(), before.width() / 2);
        QCOMPARE(p.height(), before.height() / 2);
    }
};
QTEST_MAIN(PinTests)
#include "widget_tests.moc"
