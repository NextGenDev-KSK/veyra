#include <catch2/catch.hpp>

#include <cmath>
#include <vector>

#include "enhancers/MultibandWidth.h"

using namespace veyra::dsp;

namespace {
constexpr double kFs = 48000.0;
constexpr double kPi = 3.14159265358979323846;

// Goertzel magnitude of an arbitrary per-sample signal at one frequency.
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
} // namespace

TEST_CASE("MultibandWidth collapses low-band side to mono, keeps high-band mid")
{
    const int n = 16384;
    // Low band (80 Hz) is pure SIDE (out of phase); high band (6 kHz) is pure MID.
    std::vector<float> l(n), r(n);
    for (int i = 0; i < n; ++i)
    {
        const float lo = 0.5f * (float) std::sin(2.0 * kPi * 80.0 * i / kFs);
        const float hi = 0.5f * (float) std::sin(2.0 * kPi * 6000.0 * i / kFs);
        l[(size_t) i] = lo + hi;
        r[(size_t) i] = -lo + hi;
    }

    std::vector<float> sideBefore(n), midBefore(n);
    for (int i = 0; i < n; ++i)
    {
        sideBefore[(size_t) i] = (l[(size_t) i] - r[(size_t) i]) * 0.5f;
        midBefore[(size_t) i]  = (l[(size_t) i] + r[(size_t) i]) * 0.5f;
    }
    const double sideLo0 = binMag(sideBefore, n / 4, 80.0);
    const double midHi0  = binMag(midBefore, n / 4, 6000.0);

    MultibandWidth mw;
    mw.prepare(kFs);
    mw.setAmount(1.0f);
    mw.processStereo(l.data(), r.data(), n);

    std::vector<float> sideAfter(n), midAfter(n);
    for (int i = 0; i < n; ++i)
    {
        sideAfter[(size_t) i] = (l[(size_t) i] - r[(size_t) i]) * 0.5f;
        midAfter[(size_t) i]  = (l[(size_t) i] + r[(size_t) i]) * 0.5f;
    }
    const double sideLo1 = binMag(sideAfter, n / 4, 80.0);
    const double midHi1  = binMag(midAfter, n / 4, 6000.0);

    CHECK(sideLo1 < sideLo0 * 0.35);            // low side collapsed toward mono
    CHECK(midHi1  == Approx(midHi0).epsilon(0.12)); // high-band mid preserved
}

TEST_CASE("MultibandWidth at zero amount is an exact bypass")
{
    MultibandWidth mw;
    mw.prepare(kFs);
    mw.setAmount(0.0f);
    std::vector<float> l = {0.2f, -0.5f, 0.3f, 0.7f}, r = {-0.1f, 0.4f, 0.9f, -0.3f};
    const auto l0 = l, r0 = r;
    mw.processStereo(l.data(), r.data(), (int) l.size());
    CHECK(l == l0);
    CHECK(r == r0);
}
