#include "core.hpp"
#include <cmath>
#include <iostream>
#include <stdexcept>
using namespace crossshot;
int main() {
    int checks = 0;
    auto check = [&](bool ok, const char *name) {
        ++checks;
        if (!ok)
            throw std::runtime_error(name);
    };
    try {
        check(physicalCrop(10, 20, 30, 40, 2, 2, 400, 400) == Rect{20, 40, 60, 80},
              "Retina maps logical region to physical pixels");
        check(physicalCrop(-5, -5, 10, 10, 2, 2, 100, 100) == Rect{0, 0, 10, 10},
              "clip negative origin");
        check(physicalCrop(0, 0, 10, 10, 1.25, 1.5, 100, 100) == Rect{0, 0, 13, 15},
              "fractional per-axis scale covers entire selection");
        check(physicalCrop(200, 200, 10, 10, 1, 1, 100, 100) == Rect{},
              "outside rectangle is empty");
        check(physicalCrop(0, 0, 1, 1, NAN, 1, 100, 100) == Rect{}, "invalid transform rejected");
        PinState p;
        p.setScale(100);
        check(p.scale() == 5, "scale bounded");
        p.setScale(0);
        check(p.scale() == 0.1, "minimum scale");
        p.setOpacity(0);
        check(p.opacity() == 0.1, "pin cannot become irrecoverably invisible");
        p.setOpacity(3);
        check(p.opacity() == 1, "opacity bounded");
        p.setLocked(true);
        p.setScale(2);
        check(p.locked() && p.scale() == 2, "lock and scale independent");
        auto d = decodeCtc({1, 1, 0, 1, 2}, {.8f, .9f, 1, .7f, .9f}, {"", "A", "中"});
        check(d.text == "AA中", "CTC blank separates repeated letters");
        check(std::abs(d.meanConfidence - .8) < .0001, "CTC score averages emitted tokens only");
        bool threw = false;
        try {
            decodeCtc({9}, {1}, {"", "A"});
        } catch (const std::exception &) {
            threw = true;
        }
        check(threw, "bad dictionary index rejected");
        threw = false;
        try {
            decodeCtc({1}, {}, {"", "A"});
        } catch (const std::exception &) {
            threw = true;
        }
        check(threw, "mismatched score lengths rejected");
        std::cout << "PASS " << checks << " core checks\n";
        return 0;
    } catch (const std::exception &e) {
        std::cerr << "FAIL after " << checks << " checks: " << e.what() << "\n";
        return 1;
    }
}
