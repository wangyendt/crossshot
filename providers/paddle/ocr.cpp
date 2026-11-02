// P0 horizontal-text pipeline. Normalization/CTC conventions follow
// PaddleOCR / RapidOCR (Apache-2.0). This is NOT a full DB polygon postprocessor.
#include "ocr.hpp"
#include "core.hpp"
#include <QCryptographicHash>
#include <QDir>
#include <QElapsedTimer>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QSet>
#include <algorithm>
#include <array>
#include <cmath>
#include <numeric>
#include <onnxruntime_cxx_api.h>
#include <stdexcept>
namespace crossshot {
static void require(bool ok, const char *message) {
    if (!ok)
        throw std::runtime_error(message);
}
struct PaddleOcr::Impl {
    Ort::Env env{ORT_LOGGING_LEVEL_ERROR, "crossshot"};
    Ort::SessionOptions options;
    std::unique_ptr<Ort::Session> det, rec;
    std::vector<std::string> dictionary;
    QString packId;
    explicit Impl(const QString &path) {
        QFile manifest(path + "/manifest.json");
        require(manifest.open(QIODevice::ReadOnly), "missing model manifest");
        auto doc = QJsonDocument::fromJson(manifest.readAll());
        require(doc.isObject(), "bad model manifest");
        packId = doc.object()["id"].toString();
        auto files = doc.object()["files"].toArray();
        require(files.size() == 2, "expected detector and recognizer");
        QSet<QString> seenFiles;
        for (const auto &v : files) {
            auto f = v.toObject();
            QString name = f["name"].toString();
            require(name == "det.onnx" || name == "rec.onnx", "unexpected model file");
            require(!seenFiles.contains(name), "duplicate model file");
            seenFiles.insert(name);
            QFile file(path + "/" + name);
            require(file.open(QIODevice::ReadOnly), "missing model file");
            QCryptographicHash hash(QCryptographicHash::Sha256);
            require(hash.addData(&file), "cannot hash model");
            require(hash.result().toHex() == f["sha256"].toString().toLatin1(),
                    "model hash mismatch");
        }
        options.SetIntraOpNumThreads(2);
        options.SetInterOpNumThreads(1);
        options.SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_ALL);
#ifdef _WIN32
        det = std::make_unique<Ort::Session>(env, (path + "/det.onnx").toStdWString().c_str(),
                                             options);
        rec = std::make_unique<Ort::Session>(env, (path + "/rec.onnx").toStdWString().c_str(),
                                             options);
#else
        det =
            std::make_unique<Ort::Session>(env, (path + "/det.onnx").toUtf8().constData(), options);
        rec =
            std::make_unique<Ort::Session>(env, (path + "/rec.onnx").toUtf8().constData(), options);
#endif
        Ort::AllocatorWithDefaultOptions allocator;
        auto chars =
            rec->GetModelMetadata().LookupCustomMetadataMapAllocated("character", allocator);
        require(chars.get() != nullptr, "recognizer has no embedded dictionary");
        auto list = QString::fromUtf8(chars.get()).split('\n');
        if (!list.empty() && list.last().isEmpty())
            list.removeLast();
        dictionary.push_back("");
        for (auto &ch : list)
            dictionary.push_back(ch.toStdString());
        dictionary.push_back(" ");
    }
    std::vector<Ort::Value> infer(Ort::Session &session, std::vector<float> &input, int h, int w) {
        std::array<int64_t, 4> shape{1, 3, h, w};
        auto mem = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);
        auto tensor = Ort::Value::CreateTensor<float>(mem, input.data(), input.size(), shape.data(),
                                                      shape.size());
        Ort::AllocatorWithDefaultOptions alloc;
        auto in = session.GetInputNameAllocated(0, alloc);
        auto out = session.GetOutputNameAllocated(0, alloc);
        const char *ins[]{in.get()};
        const char *outs[]{out.get()};
        return session.Run(Ort::RunOptions{nullptr}, ins, &tensor, 1, outs, 1);
    }
};
PaddleOcr::PaddleOcr(const QString &path) : impl_(std::make_unique<Impl>(path)) {}
PaddleOcr::~PaddleOcr() = default;
static std::vector<float> normalized(const QImage &original, int w, int h, bool detector,
                                     int paddedWidth = 0) {
    QImage im = original.scaled(w, h, Qt::IgnoreAspectRatio, Qt::SmoothTransformation)
                    .convertToFormat(QImage::Format_RGB888);
    int pw = paddedWidth ? paddedWidth : w;
    std::vector<float> out(static_cast<size_t>(3) * pw * h, 0);
    const float means[]{.485f, .456f, .406f}, stds[]{.229f, .224f, .225f};
    for (int y = 0; y < h; ++y) {
        const uchar *row = im.constScanLine(y);
        for (int x = 0; x < w; ++x)
            for (int c = 0; c < 3; ++c) {
                // Paddle's reference OpenCV input uses BGR channels.
                float v = row[x * 3 + (2 - c)] / 255.0f;
                out[c * pw * h + y * pw + x] =
                    detector ? (v - means[c]) / stds[c] : (v - .5f) / .5f;
            }
    }
    return out;
}
QJsonObject PaddleOcr::recognize(const QImage &image) {
    require(!image.isNull() && static_cast<int64_t>(image.width()) * image.height() <= 32000000,
            "invalid image or pixel budget exceeded");
    QElapsedTimer timer;
    timer.start();
    double ratio = std::min(1.0, 960.0 / std::max(image.width(), image.height()));
    int dw = std::max(32, qRound(image.width() * ratio / 32) * 32),
        dh = std::max(32, qRound(image.height() * ratio / 32) * 32);
    auto input = normalized(image, dw, dh, true);
    auto outputs = impl_->infer(*impl_->det, input, dh, dw);
    auto shape = outputs[0].GetTensorTypeAndShapeInfo().GetShape();
    require(shape.size() == 4 && shape[0] == 1 && shape[1] == 1, "unexpected detector shape");
    int h = static_cast<int>(shape[2]), w = static_cast<int>(shape[3]);
    require(h > 0 && w > 0 && static_cast<int64_t>(h) * w <= 4000000,
            "detector map budget exceeded");
    const float *prob = outputs[0].GetTensorData<float>();
    std::vector<unsigned char> seen(w * h);
    std::vector<QRect> boxes;
    std::vector<int> queue;
    queue.reserve(w * h / 8);
    for (int y = 0; y < h; ++y)
        for (int x = 0; x < w; ++x) {
            int start = y * w + x;
            if (seen[start] || prob[start] < .3f)
                continue;
            seen[start] = 1;
            queue.clear();
            queue.push_back(start);
            int l = x, r = x, t = y, b = y;
            double score = 0;
            for (size_t i = 0; i < queue.size(); ++i) {
                int id = queue[i], xx = id % w, yy = id / w;
                l = std::min(l, xx);
                r = std::max(r, xx);
                t = std::min(t, yy);
                b = std::max(b, yy);
                score += prob[id];
                for (int dy = -1; dy <= 1; ++dy)
                    for (int dx = -1; dx <= 1; ++dx) {
                        int nx = xx + dx, ny = yy + dy;
                        if (nx < 0 || nx >= w || ny < 0 || ny >= h)
                            continue;
                        int n = ny * w + nx;
                        if (!seen[n] && prob[n] >= .3f) {
                            seen[n] = 1;
                            queue.push_back(n);
                        }
                    }
            }
            int bw = r - l + 1, bh = b - t + 1;
            if (queue.size() < 6 || bw < 3 || bh < 3 || score / queue.size() < .5)
                continue;
            // Conservative axis-aligned DB expansion; rotated/perspective text is explicitly
            // unsupported in P0.
            double expand = double(bw) * bh * 1.5 / (2 * (bw + bh));
            auto rect = physicalCrop(l - expand, t - expand, bw + 2 * expand, bh + 2 * expand,
                                     double(image.width()) / w, double(image.height()) / h,
                                     image.width(), image.height());
            if (rect.width > 0 && rect.height > 0)
                boxes.emplace_back(rect.x, rect.y, rect.width, rect.height);
            require(boxes.size() <= 256, "too many text regions for P0");
        }
    // Strict ordering first, then group nearly co-linear fragments. Avoid a non-transitive
    // comparator.
    std::sort(boxes.begin(), boxes.end(), [](const QRect &a, const QRect &b) {
        return a.top() == b.top() ? a.left() < b.left() : a.top() < b.top();
    });
    for (size_t i = 0; i < boxes.size();) {
        size_t end = i + 1;
        while (end < boxes.size() && std::abs(boxes[end].center().y() - boxes[i].center().y()) <
                                         std::min(boxes[end].height(), boxes[i].height()) / 2)
            ++end;
        std::sort(boxes.begin() + i, boxes.begin() + end,
                  [](const QRect &a, const QRect &b) { return a.left() < b.left(); });
        i = end;
    }
    double detMs = timer.nsecsElapsed() / 1e6;
    QJsonArray lines;
    QStringList texts;
    for (const auto &box : boxes) {
        QImage crop = image.copy(box);
        int rw = std::max(1, static_cast<int>(std::ceil(48.0 * crop.width() / crop.height())));
        require(rw <= 4096, "text line too wide; crop into smaller regions");
        int padded = std::max(320, rw);
        auto recInput = normalized(crop, rw, 48, false, padded);
        auto recOutput = impl_->infer(*impl_->rec, recInput, 48, padded);
        auto rs = recOutput[0].GetTensorTypeAndShapeInfo().GetShape();
        require(rs.size() == 3 && rs[0] == 1, "unexpected recognizer shape");
        int steps = static_cast<int>(rs[1]), classes = static_cast<int>(rs[2]);
        require(classes == static_cast<int>(impl_->dictionary.size()),
                "recognizer/dictionary class mismatch");
        auto data = recOutput[0].GetTensorData<float>();
        std::vector<int> ids;
        std::vector<float> scores;
        for (int s = 0; s < steps; ++s) {
            auto begin = data + s * classes;
            auto best = std::max_element(begin, begin + classes);
            ids.push_back(static_cast<int>(best - begin));
            scores.push_back(*best);
        }
        auto decoded = decodeCtc(ids, scores, impl_->dictionary);
        QString text = QString::fromStdString(decoded.text);
        if (text.isEmpty())
            continue;
        texts << text;
        lines.append(QJsonObject{{"text", text},
                                 {"score", decoded.meanConfidence},
                                 {"box", QJsonArray{box.x(), box.y(), box.width(), box.height()}}});
    }
    return {{"status", "completed"},
            {"provider", "paddle-onnx-cpu"},
            {"modelPack", impl_->packId},
            {"text", texts.join('\n')},
            {"lines", lines},
            {"detectorMs", detMs},
            {"inferenceMs", timer.nsecsElapsed() / 1e6},
            {"warning", "P0 axis-aligned DB components; no rotated/perspective text, no full "
                        "reference parity claimed"}};
}
} // namespace crossshot
