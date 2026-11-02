#include "ocr.hpp"
#include <QCommandLineParser>
#include <QCoreApplication>
#include <QElapsedTimer>
#include <QFile>
#include <QImageReader>
#include <QJsonArray>
#include <QJsonDocument>
#include <QSaveFile>
#include <iostream>
#include <onnxruntime_c_api.h>
int main(int argc, char **argv) {
    QCoreApplication app(argc, argv);
    QCoreApplication::addLibraryPath(QCoreApplication::applicationDirPath() + "/../PlugIns");
    QCommandLineParser parser;
    parser.addHelpOption();
    parser.addOptions({{{"m", "models"}, "Verified model pack directory", "dir"},
                       {{"i", "image"}, "Image file (max 32MP)", "file"},
                       {{"o", "out"}, "Output JSON file (optional)", "file"},
                       {{"r", "repeat"}, "Repeat with same loaded session (1..10)", "n", "1"}});
    parser.process(app);
    int code = 0;
    QJsonObject result;
    try {
        bool ok = false;
        int repeats = parser.value("repeat").toInt(&ok);
        if (!ok || repeats < 1 || repeats > 10)
            throw std::runtime_error("repeat must be 1..10");
        if (parser.value("models").isEmpty() || parser.value("image").isEmpty())
            throw std::runtime_error("--models and --image are required");
        QImageReader::setAllocationLimit(128);
        QImageReader reader(parser.value("image"));
        auto size = reader.size();
        if (!size.isValid() || static_cast<qint64>(size.width()) * size.height() > 32000000)
            throw std::runtime_error("image exceeds 32MP or invalid header");
        auto image = reader.read();
        if (image.isNull())
            throw std::runtime_error("image decoding failed");
        QElapsedTimer clock;
        clock.start();
        crossshot::PaddleOcr ocr(parser.value("models"));
        double loadMs = clock.nsecsElapsed() / 1e6;
        QJsonArray timings;
        for (int i = 0; i < repeats; ++i) {
            result = ocr.recognize(image);
            timings.append(result["inferenceMs"]);
        }
        result["modelLoadMs"] = loadMs;
        result["runsMs"] = timings;
        result["onnxRuntime"] = QString::fromUtf8(OrtGetApiBase()->GetVersionString());
        result["imageWidth"] = image.width();
        result["imageHeight"] = image.height();
    } catch (const std::exception &e) {
        result = {{"status", "failed"}, {"error", QString::fromUtf8(e.what())}};
        code = 1;
    }
    auto json = QJsonDocument(result).toJson(QJsonDocument::Indented);
    if (!parser.value("out").isEmpty()) {
        QSaveFile file(parser.value("out"));
        if (!file.open(QIODevice::WriteOnly) || file.write(json) != json.size() || !file.commit()) {
            std::cerr << "cannot write output\n";
            return 2;
        }
    }
    std::cout << json.constData() << std::flush;
    return code;
}
