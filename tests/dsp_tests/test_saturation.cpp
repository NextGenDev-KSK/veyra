#include <catch2/catch.hpp>

#include <cmath>
#include <vector>

#include "enhancers/Saturator.h"

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

// Returns {2nd-harmonic, 3rd-harmonic} magnitudes for a 1 kHz tone.
std::pair<double, double> harmonics(int mode, float amount)
{
    Saturator s;
    s.prepare(kFs);
    s.setMode(mode);
    s.setAmount(amount);
    const int n = 8192;
    std::vector<float> l(n), r(n);
    for (int i = 0; i < n; ++i)
        l[(size_t) i] = r[(size_t) i] = 0.8f * (float) std::sin(2.0 * kPi * 1000.0 * i / kFs);
    s.processStereo(l.data(), r.data(), n);
    return {binMag(l, n / 4, 2000.0), binMag(l, n / 4, 3000.0)};
}
} // namespace

TEST_CASE("Saturator transparent mode adds odd (3rd) harmonics, little even")
{
    auto [h2, h3] = harmonics(0, 0.9f);
    CHECK(h3 > 1.0);        // clear odd-harmonic content
    CHECK(h3 > h2 * 4.0);   // symmetric curve -> 3rd dominates 2nd
}

TEST_CASE("Saturator tube mode adds even (2nd) harmonics")
{
    auto tube = harmonics(2, 0.9f);
    auto clean = harmonics(0, 0.9f);
    CHECK(tube.first > 1.0);                 // 2nd harmonic present
    CHECK(tube.first > clean.first * 4.0);   // far more even harmonics than transparent
}

TEST_CASE("Saturator 2x oversampling reduces aliasing")
{
    // 13 kHz tone, transparent: the 3rd harmonic (39 kHz) folds to 9 kHz at 48 k.
    auto aliasBin = [](bool oversample)
    {
        Saturator s;
        s.prepare(kFs);
        s.setMode(0);
        s.setAmount(0.9f);
        s.setOversample(oversample);
        const int n = 16384;
        std::vector<float> l(n), r(n);
        for (int i = 0; i < n; ++i)
            l[(size_t) i] = r[(size_t) i] = 0.6f * (float) std::sin(2.0 * kPi * 13000.0 * i / kFs);
        s.processStereo(l.data(), r.data(), n);
        return binMag(l, n / 4, 9000.0);
    };
    const double off = aliasBin(false);
    const double on  = aliasBin(true);
    CHECK(off > 1.0);        // aliased image present at the original rate
    CHECK(on < off * 0.7);   // oversampling filters it out before decimation
}

TEST_CASE("Saturator at zero amount is an exact bypass")
{
    Saturator s;
    s.prepare(kFs);
    s.setAmount(0.0f);
    std::vector<float> l = {0.2f, -0.5f, 0.9f, -0.1f}, r = l;
    const auto l0 = l;
    s.processStereo(l.data(), r.data(), (int) l.size());
    CHECK(l == l0);
}
