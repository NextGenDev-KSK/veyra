#include <catch2/catch.hpp>

#include <cmath>
#include <vector>

#include "enhancers/BassEnhancer.h"

using namespace veyra::dsp;

namespace {
constexpr double kFs = 48000.0;
constexpr double kPi = 3.14159265358979323846;

double binMag(const std::vector<float>& x, int from, double hz)
{
    const double w = 2.0 * kPi * hz / kFs;
    const double cw = std::cos(w), sw = std::sin(w), coeff = 2.0 * cw;
    double s1 = 0.0, s2 = 0.0;
    for (int i = from; i < (int) x.size(); ++i)
    {
        const double s0 = (double) x[(size_t) i] + coeff * s1 - s2;
        s2 = s1;
        s1 = s0;
    }
    const double re = s1 - s2 * cw, im = s2 * sw;
    return std::sqrt(re * re + im * im);
}

double octaveHarmonic(float amount)
{
    BassEnhancer b;
    b.prepare(kFs);
    b.setAmount(amount);
    const int n = 16384;
    std::vector<float> l(n), r(n);
    for (int i = 0; i < n; ++i)
        l[(size_t) i] = r[(size_t) i] = 0.5f * (float) std::sin(2.0 * kPi * 50.0 * i / kFs);
    b.processStereo(l.data(), r.data(), n);
    return binMag(l, n / 4, 100.0); // 2nd harmonic (one octave up)
}
} // namespace

TEST_CASE("BassEnhancer synthesises an octave-up harmonic from the low fundamental")
{
    const double off = octaveHarmonic(0.0f);
    const double on  = octaveHarmonic(0.9f);
    CHECK(on > off * 10.0 + 1.0); // harmonic bass appears
}

TEST_CASE("BassEnhancer at zero amount is an exact bypass")
{
    BassEnhancer b;
    b.prepare(kFs);
    b.setAmount(0.0f);
    std::vector<float> l = {0.2f, -0.5f, 0.4f, -0.1f}, r = l;
    const auto l0 = l;
    b.processStereo(l.data(), r.data(), (int) l.size());
    CHECK(l == l0);
}
