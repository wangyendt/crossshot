#include "capture.hpp"
#include "core.hpp"
#include "fixture.hpp"
#include "pin.hpp"
#include <QApplication>
#include <QComboBox>
#include <QCommandLineParser>
#include <QDir>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QImageReader>
#include <QJsonArray>
#include <QJsonDocument>
#include <QKeyEvent>
#include <QLabel>
#include <QMainWindow>
#include <QMessageBox>
#include <QMouseEvent>
#include <QPainter>
#include <QPlainTextEdit>
#include <QPointer>
#include <QProcess>
#include <QPushButton>
#include <QSaveFile>
#include <QScreen>
#include <QSysInfo>
#include <QTemporaryDir>
#include <QTimer>
#include <QVBoxLayout>
#include <QtTest>
using namespace crossshot;
static bool writeJson(const QString &path, const QJsonObject &data) {
    QSaveFile f(path);
    auto bytes = QJsonDocument(data).toJson(QJsonDocument::Indented);
    return f.open(QIODevice::WriteOnly) && f.write(bytes) == bytes.size() && f.commit();
}
class Selection : public QWidget {
    QImage image_;
    QPoint begin_, end_;
    bool selecting_{};
    std::function<void(QImage)> done_;

  public:
    Selection(QImage image, std::function<void(QImage)> done)
        : QWidget(nullptr, Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint),
          image_(std::move(image)), done_(std::move(done)) {
        setAttribute(Qt::WA_DeleteOnClose);
        setGeometry(QGuiApplication::primaryScreen()->geometry());
        setCursor(Qt::CrossCursor);
        setMouseTracking(true);
        show();
        raise();
        activateWindow();
        setFocus();
    }
    void paintEvent(QPaintEvent *) override {
        QPainter p(this);
        p.drawImage(rect(), image_);
        QRegion mask(rect());
        if (selecting_)
            mask -= QRect(begin_, end_).normalized();
        p.setClipRegion(mask);
        p.fillRect(rect(), QColor(0, 0, 0, 90));
        p.setClipping(false);
        if (selecting_) {
            p.setPen(QPen(Qt::cyan, 2));
            p.drawRect(QRect(begin_, end_).normalized());
        }
        p.setPen(Qt::white);
        p.drawText(20, 30, QString::fromUtf8("拖动选择区域 · Esc 取消 · P0 仅主屏幕"));
    }
    void mousePressEvent(QMouseEvent *e) override {
        if (e->button() == Qt::LeftButton) {
            begin_ = end_ = e->position().toPoint();
            selecting_ = true;
            update();
        }
    }
    void mouseMoveEvent(QMouseEvent *e) override {
        if (selecting_) {
            end_ = e->position().toPoint();
            update();
        }
    }
    void mouseReleaseEvent(QMouseEvent *e) override {
        if (!selecting_ || e->button() != Qt::LeftButton)
            return;
        end_ = e->position().toPoint();
        auto r = QRect(begin_, end_).normalized();
        auto c = physicalCrop(r.x(), r.y(), r.width(), r.height(), double(image_.width()) / width(),
                              double(image_.height()) / height(), image_.width(), image_.height());
        QImage image;
        if (c.width && c.height)
            image = image_.copy(c.x, c.y, c.width, c.height);
        done_(image);
        done_ = {};
        close();
    }
    void keyPressEvent(QKeyEvent *e) override {
        if (e->key() == Qt::Key_Escape) {
            done_({});
            done_ = {};
            close();
        }
    }
};
static QString repositoryRoot() {
    auto env = qEnvironmentVariable("CROSSSHOT_ROOT");
    if (!env.isEmpty())
        return env;
    QDir dir(QCoreApplication::applicationDirPath());
    for (int i = 0; i < 7; ++i) {
        if (dir.exists("tools/models.lock.json"))
            return dir.absolutePath();
        if (!dir.cdUp())
            break;
    }
    return {};
}
static QString workerPath() {
    QString name = "crossshot-ocr";
#ifdef Q_OS_WIN
    name += ".exe";
#endif
    return QDir(QCoreApplication::applicationDirPath()).filePath(name);
}
class MainWindow : public QMainWindow {
    QImage image_;
    QLabel *preview_;
    QPlainTextEdit *text_;
    QComboBox *models_;
    QPushButton *recognize_;
    QPushButton *cancel_;
    QProcess *process_{};
    QTemporaryDir temp_;
    QString root_;
    QList<QPointer<PinWindow>> pins_;
    QByteArray response_;
    quint64 generation_{};

  public:
    MainWindow() : root_(repositoryRoot()) {
        setWindowTitle("CrossShot · P0 / Mac ARM");
        resize(850, 680);
        auto central = new QWidget;
        setCentralWidget(central);
        auto layout = new QVBoxLayout(central);
        layout->addWidget(new QLabel(
            "<h2>CrossShot · P0 风险验证</h2>截图 / 图片贴图 / 本地 PaddleOCR · 尚非正式版"));
        auto row = new QHBoxLayout;
        layout->addLayout(row);
        auto add = [row, this](QString title, auto fn) {
            auto b = new QPushButton(title);
            row->addWidget(b);
            connect(b, &QPushButton::clicked, this, fn);
        };
        add("截图主屏幕", [this] {
#ifndef Q_OS_MACOS
            QMessageBox::information(
                this, "能力尚未实现",
                "P0 仅实现 macOS 原生捕获；Windows/Linux 后端尚未实现，不会以模拟截图替代。");
            return;
#endif
            if (!screenPermission()) {
                requestScreenPermission();
                if (!screenPermission()) {
                    QMessageBox::information(
                        this, "屏幕录制权限",
                        "请在系统设置 → 隐私与安全性 → 屏幕与系统音频录制中允许 "
                        "CrossShot，然后完全退出并重新打开。不会将未授权的空白画面当作截图。");
                    return;
                }
            }
            hide();
            QTimer::singleShot(350, this, [this] {
                QPointer<MainWindow> self(this);
                capturePrimary([self](CaptureResult r) {
                    if (!self)
                        return;
                    if (!r.error.isEmpty()) {
                        self->show();
                        QMessageBox::warning(self, "截图失败", r.error);
                        return;
                    }
                    new Selection(r.image, [self](QImage image) {
                        if (self) {
                            self->show();
                            if (!image.isNull())
                                self->setImage(image);
                        }
                    });
                });
            });
        });
        add("打开图片", [this] {
            auto path = QFileDialog::getOpenFileName(this, "选择本地图片", {},
                                                     "Images (*.png *.jpg *.jpeg *.bmp *.webp)");
            if (path.isEmpty())
                return;
            QImageReader reader(path);
            auto s = reader.size();
            if (!s.isValid() || static_cast<qint64>(s.width()) * s.height() > 32000000) {
                QMessageBox::warning(this, "图像限制", "P0 仅支持最多3200万像素图片");
                return;
            }
            auto image = reader.read();
            if (!image.isNull())
                setImage(image);
        });
        add("测试图", [this] { setImage(makeFixture()); });
        add("贴图", [this] {
            if (image_.isNull())
                return;
            auto pin = new PinWindow(image_);
            pins_.append(pin);
            pin->move(120, 120);
            pin->show();
            configureNativePin(pin);
        });
        add("恢复全部贴图", [this] {
            for (auto &pin : pins_)
                if (pin) {
                    pin->setPinOpacity(1);
                    pin->setLocked(false);
                    pin->move(120, 120);
                    pin->show();
                    pin->raise();
                }
        });
        add("保存 PNG", [this] {
            if (image_.isNull())
                return;
            auto path = QFileDialog::getSaveFileName(this, "保存截图", {}, "PNG (*.png)");
            if (!path.isEmpty() && !image_.save(path, "PNG"))
                QMessageBox::warning(this, "保存失败", path);
        });
        preview_ = new QLabel;
        preview_->setAlignment(Qt::AlignCenter);
        preview_->setMinimumHeight(220);
        layout->addWidget(preview_);
        auto ocrRow = new QHBoxLayout;
        layout->addLayout(ocrRow);
        models_ = new QComboBox;
        for (auto id : {"ppocr-v4-mobile", "ppocr-v5-mobile"})
            if (QFile::exists(root_ + "/models/" + id + "/manifest.json"))
                models_->addItem(id);
        ocrRow->addWidget(new QLabel("本地模型"));
        ocrRow->addWidget(models_, 1);
        recognize_ = new QPushButton("本地识别");
        cancel_ = new QPushButton("取消");
        cancel_->setEnabled(false);
        ocrRow->addWidget(recognize_);
        ocrRow->addWidget(cancel_);
        text_ = new QPlainTextEdit;
        text_->setReadOnly(true);
        layout->addWidget(text_, 1);
        layout->addWidget(new QLabel("隐私：识别仅运行本地 C++ CPU worker；此探针无远程 "
                                     "OCR、自动上传或遥测。\n贴图：滚轮缩放，Ctrl+"
                                     "滚轮透明度，右键锁定/关闭。退出应用会关闭贴图。"));
        connect(recognize_, &QPushButton::clicked, this, [this] { startOcr(); });
        connect(cancel_, &QPushButton::clicked, this, [this] {
            if (process_)
                process_->kill();
        });
        setImage(makeFixture());
        if (models_->count() == 0)
            text_->setPlainText("未找到模型，请先运行 tools/fetch_models.py；不会自动联网下载。");
    }
    ~MainWindow() {
        if (process_) {
            process_->disconnect(this);
            process_->kill();
            process_->waitForFinished(2000);
        }
        for (auto &pin : pins_)
            if (pin)
                pin->close();
    }
    void setImage(QImage im) {
        ++generation_;
        text_->clear();
        if (process_)
            process_->kill();
        image_ = std::move(im);
        preview_->setPixmap(QPixmap::fromImage(image_).scaled(800, 260, Qt::KeepAspectRatio,
                                                              Qt::SmoothTransformation));
    }
    void startOcr() {
        if (process_ || image_.isNull() || models_->currentText().isEmpty())
            return;
        if (!temp_.isValid()) {
            text_->setPlainText("无法创建私有临时目录");
            return;
        }
        auto path = temp_.filePath("input.png");
        if (!image_.save(path, "PNG")) {
            text_->setPlainText("保存输入失败");
            return;
        }
        QFile::setPermissions(path, QFile::ReadOwner | QFile::WriteOwner);
        response_.clear();
        const auto generation = generation_;
        process_ = new QProcess(this);
        recognize_->setEnabled(false);
        cancel_->setEnabled(true);
        models_->setEnabled(false);
        text_->setPlainText("识别中（本地 CPU）…");
        auto timeout = new QTimer(process_);
        timeout->setSingleShot(true);
        connect(timeout, &QTimer::timeout, process_, &QProcess::kill);
        timeout->start(120000);
        connect(process_, &QProcess::readyReadStandardOutput, this, [this] {
            response_ += process_->readAllStandardOutput();
            if (response_.size() > 4 * 1024 * 1024)
                process_->kill();
        });
        connect(process_, &QProcess::errorOccurred, this, [this](QProcess::ProcessError error) {
            if (error == QProcess::FailedToStart) {
                text_->setPlainText("无法启动本地 OCR worker：" + process_->errorString());
                finishWorker();
            }
        });
        connect(
            process_, qOverload<int, QProcess::ExitStatus>(&QProcess::finished), this,
            [this, generation](int code, QProcess::ExitStatus status) {
                if (generation != generation_) {
                    finishWorker();
                    return;
                }
                response_ += process_->readAllStandardOutput();
                auto json = QJsonDocument::fromJson(response_).object();
                if (status == QProcess::NormalExit && code == 0 && json["status"] == "completed")
                    text_->setPlainText(json["text"].toString() + "\n\n模型：" +
                                        json["modelPack"].toString() + "\n推理耗时：" +
                                        QString::number(json["inferenceMs"].toDouble(), 'f', 1) +
                                        " ms\nP0 限制：横向文字；无旋转/透视校正。");
                else
                    text_->setPlainText("识别未完成（取消/超时/错误）：" +
                                        json["error"].toString());
                finishWorker();
            });
        process_->start(workerPath(),
                        {"--models", root_ + "/models/" + models_->currentText(), "--image", path});
    }
    void finishWorker() {
        if (!process_)
            return;
        process_->deleteLater();
        process_ = nullptr;
        recognize_->setEnabled(true);
        models_->setEnabled(true);
        cancel_->setEnabled(false);
        QFile::remove(temp_.filePath("input.png"));
    }
};
static void runProbe(const QString &output) {
    QDir().mkpath(output);
    auto fixture = makeFixture();
    fixture.save(output + "/fixture.png");
    auto screen = QGuiApplication::primaryScreen();
    auto window = new QLabel(nullptr, Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
    window->setWindowTitle("CrossShot P0 synthetic fixture");
    window->setScaledContents(true);
    window->setPixmap(QPixmap::fromImage(fixture));
    window->setFixedSize(600, 210);
    window->move(screen->geometry().topLeft() + QPoint(60, 100));
    window->show();
    window->raise();
    auto pin = new PinWindow(fixture);
    pin->move(screen->geometry().topLeft() + QPoint(80, 400));
    pin->show();
    configureNativePin(pin);
    QTimer::singleShot(1200, [output, window, pin, screen] {
        QJsonObject report{
            {"os", QSysInfo::prettyProductName()},
            {"architecture", QSysInfo::currentCpuArchitecture()},
            {"qt", QT_VERSION_STR},
            {"screenName", screen->name()},
            {"screenDpr", screen->devicePixelRatio()},
            {"logicalScreen", QJsonArray{screen->geometry().x(), screen->geometry().y(),
                                         screen->geometry().width(), screen->geometry().height()}},
            {"screenPermission", screenPermission()}};
        pin->setZoom(1);
        auto full = pin->size();
        pin->setZoom(.5);
        auto half = pin->size();
        pin->setPinOpacity(.55);
        pin->setLocked(true);
        auto native = nativePinInfo(pin);
        auto originalPos = pin->pos();
        QTest::mousePress(pin, Qt::LeftButton, {}, QPoint(10, 10));
        QTest::mouseMove(pin, QPoint(80, 80));
        QTest::mouseRelease(pin, Qt::LeftButton, {}, QPoint(80, 80));
        report["pin"] = QJsonObject{{"zoomGeometry", half.width() == full.width() / 2 &&
                                                         half.height() == full.height() / 2},
                                    {"lockedDrag", pin->pos() == originalPos},
                                    {"qtOpacity", pin->windowOpacity()},
                                    {"native", native},
                                    {"visualOcclusionVerified", false},
                                    {"spacesVerified", false}};
        auto occluder = new QLabel(nullptr, Qt::FramelessWindowHint);
        occluder->setStyleSheet("background:black;");
        occluder->setGeometry(pin->geometry());
        occluder->show();
        occluder->raise();
        auto finish = [output, window, pin, occluder, report,
                       screen](CaptureResult result) mutable {
            if (!result.error.isEmpty()) {
                report["capture"] = QJsonObject{{"status", "blocked"}, {"reason", result.error}};
                writeJson(output + "/mac-probe.json", report);
                pin->close();
                window->close();
                occluder->close();
                qApp->exit(3);
                return;
            }
            QPoint origin = window->mapToGlobal(QPoint(0, 0)) - screen->geometry().topLeft();
            double sx = double(result.image.width()) / screen->geometry().width(),
                   sy = double(result.image.height()) / screen->geometry().height();
            auto r = physicalCrop(origin.x(), origin.y(), window->width(), window->height(), sx, sy,
                                  result.image.width(), result.image.height());
            auto crop = result.image.copy(r.x, r.y, r.width, r.height);
            bool anchors = false;
            if (crop.width() > 40 && crop.height() > 40) {
                auto tl = crop.pixelColor(5, 5), tr = crop.pixelColor(crop.width() - 6, 5),
                     bl = crop.pixelColor(5, crop.height() - 6),
                     br = crop.pixelColor(crop.width() - 6, crop.height() - 6);
                anchors = tl.red() > 180 && tl.green() < 80 && tr.green() > 180 && tr.red() < 80 &&
                          bl.blue() > 180 && bl.red() < 80 && br.red() > 180 && br.green() > 180;
            }
            bool nativePixels = qAbs(sx - screen->devicePixelRatio()) < .01 &&
                                qAbs(sy - screen->devicePixelRatio()) < .01;
            auto sample = pin->mapToGlobal(QPoint(pin->width() - 12, pin->height() / 2)) -
                          screen->geometry().topLeft();
            auto color = result.image.pixelColor(
                qBound(0, qRound(sample.x() * sx), result.image.width() - 1),
                qBound(0, qRound(sample.y() * sy), result.image.height() - 1));
            bool compositedPin =
                color.red() > 40 && color.red() < 245 && color.green() > 40 && color.blue() > 40;
            auto pinReport = report["pin"].toObject();
            pinReport["visualOcclusionVerified"] = compositedPin;
            pinReport["compositedRgb"] = QJsonArray{color.red(), color.green(), color.blue()};
            report["pin"] = pinReport;
            if (anchors)
                crop.save(output + "/captured-fixture.png");
            report["capture"] = QJsonObject{
                {"status", anchors && nativePixels ? "passed" : "failed"},
                {"nativeBackend", "ScreenCaptureKit/SCScreenshotManager"},
                {"elapsedMs", result.elapsedMs},
                {"physicalFrame", QJsonArray{result.image.width(), result.image.height()}},
                {"cropPhysical", QJsonArray{crop.width(), crop.height()}},
                {"cornerAnchors", anchors},
                {"matchesQtDevicePixelRatio", nativePixels},
                {"pixelScale", sx},
                {"fullDesktopPersisted", false}};
            writeJson(output + "/mac-probe.json", report);
            pin->close();
            window->close();
            occluder->close();
            qApp->exit(anchors && nativePixels && compositedPin ? 0 : 4);
        };
        QTimer::singleShot(600, [finish] { capturePrimary(finish); });
    });
}
static void runUiSmoke(MainWindow *window, const QString &output) {
    QDir().mkpath(output);
    auto combo = window->findChild<QComboBox *>();
    auto text = window->findChild<QPlainTextEdit *>();
    QPushButton *run = nullptr;
    QPushButton *cancel = nullptr;
    for (auto b : window->findChildren<QPushButton *>()) {
        if (b->text() == "本地识别")
            run = b;
        if (b->text() == "取消")
            cancel = b;
    }
    if (!combo || combo->count() != 2 || !run || !cancel || !text) {
        writeJson(output + "/ui-smoke.json",
                  {{"status", "failed"}, {"reason", "missing widgets/models"}});
        qApp->exit(6);
        return;
    }
    auto state = std::make_shared<QJsonObject>();
    auto phase = std::make_shared<int>(0);
    auto ticks = std::make_shared<int>(0);
    auto timer = new QTimer(window);
    timer->setInterval(50);
    combo->setCurrentIndex(0);
    run->click();
    QObject::connect(timer, &QTimer::timeout, window, [=] {
        ++*ticks;
        if (*ticks > 400) {
            writeJson(output + "/ui-smoke.json", {{"status", "failed"}, {"reason", "timeout"}});
            qApp->exit(6);
            return;
        }
        if (*phase < 2 && run->isEnabled() &&
            text->toPlainText().contains("Invoice 12345.67 ABC xyz") &&
            text->toPlainText().contains(combo->currentText())) {
            (*state)[combo->currentText()] = true;
            if (*phase == 0) {
                ++*phase;
                combo->setCurrentIndex(1);
                run->click();
            } else {
                ++*phase;
                window->grab().save(output + "/app-ui.png");
                run->click();
                QTimer::singleShot(50, cancel, &QPushButton::click);
            }
        } else if (*phase == 2 && run->isEnabled() &&
                   text->toPlainText().startsWith("识别未完成")) {
            (*state)["cancelledWorkerRecovered"] = true;
            ++*phase;
            run->click();
            QTimer::singleShot(50, window, [window] { window->setImage(makeFixture()); });
        } else if (*phase == 3 && run->isEnabled() && text->toPlainText().isEmpty()) {
            (*state)["status"] = "passed";
            (*state)["staleResultSuppressed"] = true;
            (*state)["heartbeatTicks"] = *ticks;
            writeJson(output + "/ui-smoke.json", *state);
            timer->stop();
            qApp->exit(0);
        }
    });
    timer->start();
}

int main(int argc, char **argv) {
    QApplication app(argc, argv);
    app.setApplicationName("CrossShot");
    app.setOrganizationName("CrossShot");
    QImageReader::setAllocationLimit(128);
    QCommandLineParser parser;
    parser.addHelpOption();
    parser.addOptions(
        {{"probe", "Run native capture/pin probe; save only synthetic fixture", "directory"},
         {"ui-smoke", "Exercise GUI model switching and cancellation with synthetic image",
          "directory"},
         {"fixture", "Write synthetic image and exit", "file"},
         {"request-permission", "Ask macOS for screen capture permission"}});
    parser.process(app);
    if (parser.isSet("request-permission")) {
        requestScreenPermission();
        return 0;
    }
    if (parser.isSet("fixture"))
        return makeFixture().save(parser.value("fixture")) ? 0 : 1;
    if (parser.isSet("probe")) {
        QTimer::singleShot(0, [&] { runProbe(QDir(parser.value("probe")).absolutePath()); });
        QTimer::singleShot(30000, &app, [] { qApp->exit(5); });
        return app.exec();
    }
    MainWindow window;
    window.show();
    if (parser.isSet("ui-smoke"))
        QTimer::singleShot(
            300, [&] { runUiSmoke(&window, QDir(parser.value("ui-smoke")).absolutePath()); });
    return app.exec();
}
