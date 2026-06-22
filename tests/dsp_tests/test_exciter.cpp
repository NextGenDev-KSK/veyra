#include <catch2/catch.hpp>

#include <cmath>
#include <vector>

#include "enhancers/Exciter.h"

using namespace veyra::dsp;

namespace {
constexpr double kFs = 48000.0;
constexpr double kPi = 3.14159265358979323846;

// Goertzel single-bin magnitude over the settled tail.
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

double secondHarmonic(float amount)
{
    Exciter e;
    e.prepare(kFs);
    e.setAmount(amount);
    const int n = 8192;
    std::vector<float> l(n), r(n);
    for (int i = 0; i < n; ++i)
        l[(size_t) i] = r[(size_t) i] = 0.5f * (float) std::sin(2.0 * kPi * 5000.0 * i / kFs);
    e.processStereo(l.data(), r.data(), n);
    return binMag(l, n / 4, 10000.0); // 2nd harmonic of 5 kHz
}
} // namespace

TEST_CASE("Exciter synthesises upper harmonics scaled by amount")
{
    const double off = secondHarmonic(0.0f);
    const double on  = secondHarmonic(0.9f);
    CHECK(on > off * 10.0 + 1.0); // clear 2nd-harmonic energy appears
}

TEST_CASE("Exciter at zero is an exact bypass")
{
    Exciter e;
    e.prepare(kFs);
    e.setAmount(0.0f);
    std::vector<float> l = {0.1f, -0.4f, 0.7f, -0.2f}, r = l;
    const auto l0 = l;
    e.processStereo(l.data(), r.data(), (int) l.size());
    CHECK(l == l0);
}
