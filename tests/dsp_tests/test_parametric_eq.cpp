#include <catch2/catch.hpp>

#include <array>
#include <cmath>
#include <vector>

#include "eq/ParametricEq.h"

using namespace veyra::dsp;

namespace {
constexpr double kFs = 48000.0;
constexpr double kPi = 3.14159265358979323846;

// RMS of a unit sine at `hz` after passing through `eq` (settled second half).
float rmsThrough(ParametricEq& eq, double hz)
{
    const int n = 8192;
    std::vector<float> l(n), r(n);
    for (int i = 0; i < n; ++i)
        l[i] = r[i] = (float) std::sin(2.0 * kPi * hz * i / kFs);
    eq.processStereo(l.data(), r.data(), n);
    double acc = 0.0;
    for (int i = n / 2; i < n; ++i) acc += (double) l[i] * l[i];
    return (float) std::sqrt(acc / (n / 2));
}

std::array<EqBand, ParametricEq::kMaxBands> oneBand(EqBandType t, float f, float g, float q)
{
    std::array<EqBand, ParametricEq::kMaxBands> b{};
    b[0] = EqBand{true, t, f, g, q};
    return b;
}
} // namespace

TEST_CASE("ParametricEq: disabled is passthrough")
{
    ParametricEq eq;
    eq.prepare(kFs);
    eq.setBands(oneBand(EqBandType::Bell, 1000.0f, 12.0f, 1.0f));
    eq.setEnabled(false);
    std::vector<float> l{0.5f, -0.5f, 0.25f}, r{0.5f, -0.5f, 0.25f};
    eq.processStereo(l.data(), r.data(), 3);
    CHECK(l[0] == Approx(0.5f));
    CHECK(l[2] == Approx(0.25f));
}

TEST_CASE("ParametricEq: a bell boost lifts its centre band")
{
    ParametricEq eq;
    eq.prepare(kFs);
    eq.setEnabled(true);
    eq.setBands(oneBand(EqBandType::Bell, 1000.0f, 12.0f, 1.0f));

    const float at1k  = rmsThrough(eq, 1000.0);
    eq.prepare(kFs); // reset state
    eq.setEnabled(true);
    eq.setBands(oneBand(EqBandType::Bell, 1000.0f, 12.0f, 1.0f));
    const float at100 = rmsThrough(eq, 100.0);

    CHECK(at1k > 1.5f);   // ~+12 dB at centre (unit sine RMS ~0.707 -> ~2.8)
    CHECK(at100 < 1.2f);  // far from centre: roughly untouched
}

TEST_CASE("ParametricEq: high-pass attenuates lows, passes highs")
{
    ParametricEq eq;
    eq.prepare(kFs);
    eq.setEnabled(true);
    eq.setBands(oneBand(EqBandType::HighPass, 1000.0f, 0.0f, 0.707f));
    const float low = rmsThrough(eq, 100.0);

    eq.prepare(kFs);
    eq.setEnabled(true);
    eq.setBands(oneBand(EqBandType::HighPass, 1000.0f, 0.0f, 0.707f));
    const float high = rmsThrough(eq, 8000.0);

    CHECK(low < 0.2f);    // 100 Hz well below 1 kHz cutoff -> strongly cut
    CHECK(high > 0.6f);   // 8 kHz passes ~unchanged (unit RMS ~0.707)
}

TEST_CASE("ParametricEq: notch deeply attenuates its frequency")
{
    ParametricEq eq;
    eq.prepare(kFs);
    eq.setEnabled(true);
    eq.setBands(oneBand(EqBandType::Notch, 1000.0f, 0.0f, 4.0f));
    CHECK(rmsThrough(eq, 1000.0) < 0.3f);
}
