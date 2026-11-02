#include "core.hpp"
#include <algorithm>
#include <cmath>
#include <stdexcept>
namespace crossshot {
Rect physicalCrop(double x, double y, double w, double h, double sx, double sy, int width,
                  int height) {
    if (!std::isfinite(x) || !std::isfinite(y) || !std::isfinite(w) || !std::isfinite(h) ||
        !std::isfinite(sx) || !std::isfinite(sy) || sx <= 0 || sy <= 0 || w <= 0 || h <= 0 ||
        width <= 0 || height <= 0)
        return {};
    auto bound = [](double v, int max) {
        return static_cast<int>(std::clamp(v, 0.0, static_cast<double>(max)));
    };
    int l = bound(std::floor(x * sx), width), t = bound(std::floor(y * sy), height);
    int rr = bound(std::ceil((x + w) * sx), width), b = bound(std::ceil((y + h) * sy), height);
    if (rr <= l || b <= t)
        return {};
    return {l, t, rr - l, b - t};
}
void PinState::setScale(double v) {
    if (std::isfinite(v))
        scale_ = std::clamp(v, 0.1, 5.0);
}
void PinState::setOpacity(double v) {
    if (std::isfinite(v))
        opacity_ = std::clamp(v, 0.1, 1.0);
}
Decoded decodeCtc(const std::vector<int> &indices, const std::vector<float> &scores,
                  const std::vector<std::string> &dict) {
    if (indices.size() != scores.size() || dict.empty())
        throw std::invalid_argument("invalid CTC input");
    Decoded out;
    int previous = -1, count = 0;
    double sum = 0;
    for (size_t i = 0; i < indices.size(); ++i) {
        int id = indices[i];
        if (id < 0 || static_cast<size_t>(id) >= dict.size())
            throw std::invalid_argument("dictionary mismatch");
        if (id != 0 && id != previous) {
            out.text += dict[id];
            sum += scores[i];
            ++count;
        }
        previous = id;
    }
    out.meanConfidence = count ? sum / count : 0;
    return out;
}
} // namespace crossshot
