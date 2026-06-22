#include <catch2/catch.hpp>

#include <cmath>
#include <vector>

#include "enhancers/FieldCompensation.h"

using namespace veyra::dsp;

namespace {
constexpr double kFs = 48000.0;
constexpr double kPi = 3.14159265358979323846;

float rms(int mode, double hz)
{
    FieldCompensation f;
    f.prepare(kFs);
    f.setMode(mode);
    const int n = 8192;
    std::vector<float> l(n), r(n);
    for (int i = 0; i < n; ++i)
        l[(size_t) i] = r[(size_t) i] = (float) std::sin(2.0 * kPi * hz * i / kFs);
    f.processStereo(l.data(), r.data(), n);
    double acc = 0.0;
    for (int i = n / 2; i < n; ++i) acc += (double) l[(size_t) i] * l[(size_t) i];
    return (float) std::sqrt(acc / (n / 2));
}
} // namespace

TEST_CASE("FieldCompensation: free-field lifts the 3 kHz presence, diffuse relaxes it")
{
    const float ref = rms(0, 3000.0); // mode 0 = bypass reference
    CHECK(rms(2, 3000.0) > ref * 1.2f);  // free-field: forward presence boost
    CHECK(rms(1, 3000.0) < ref * 0.95f); // diffuse: gentle relax
    CHECK(rms(2, 1000.0) == Approx(ref).margin(0.03f)); // mids ~untouched
}

TEST_CASE("FieldCompensation: mode 0 is an exact bypass")
{
    FieldCompensation f;
    f.prepare(kFs);
    f.setMode(0);
    std::vector<float> l = {0.2f, -0.5f, 0.7f, -0.3f}, r = l;
    const auto l0 = l;
    f.processStereo(l.data(), r.data(), (int) l.size());
    CHECK(l == l0);
}
