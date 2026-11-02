#pragma once
#include <string>
#include <vector>
namespace crossshot {
struct Rect {
    int x{}, y{}, width{}, height{};
    bool operator==(const Rect &) const = default;
};
// Logical local rectangle to physical pixels, clipped to image. Origin is screen-local.
Rect physicalCrop(double x, double y, double w, double h, double sx, double sy, int width,
                  int height);
class PinState {
    double scale_{1.0}, opacity_{1.0};
    bool locked_{false};

  public:
    void setScale(double);
    double scale() const {
        return scale_;
    }
    void setOpacity(double);
    double opacity() const {
        return opacity_;
    }
    void setLocked(bool v) {
        locked_ = v;
    }
    bool locked() const {
        return locked_;
    }
};
struct Decoded {
    std::string text;
    double meanConfidence{};
};
Decoded decodeCtc(const std::vector<int> &indices, const std::vector<float> &scores,
                  const std::vector<std::string> &dictionary);
} // namespace crossshot
