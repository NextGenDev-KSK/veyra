#include <catch2/catch.hpp>

#include <vector>

#include "TestHelpers.h"
#include "dynamics/Compressor.h"
#include "dynamics/Limiter.h"

using namespace veyra::dsp;

namespace {
double steadyOutRms(float amp)
{
    Compressor c;
    c.prepare(48000.0);
    c.setAmount(1.0f); // ratio ~8:1, threshold -30 dB
    auto in = th::sine(1000.0, 48000.0, 24000, amp);
    std::vector<float> L = in, R = in;
    c.processStereo(L.data(), R.data(), static_cast<int>(L.size()));
    return th::rmsTail(L, 0.2);
}
} // namespace

TEST_CASE("Compressor: output level grows slower than input (ratio < 1:1)")
{
    const double outLow = steadyOutRms(0.1f);  // -20 dBFS in
    const double outHigh = steadyOutRms(0.4f); // -8 dBFS in (+12 dB)

    const double inDeltaDb = th::linToDb(0.4) - th::linToDb(0.1); // ~12 dB
    const double outDeltaDb = th::linToDb(outHigh) - th::linToDb(outLow);

    REQUIRE(outDeltaDb > 0.0);            // still increases
    REQUIRE(outDeltaDb < 0.5 * inDeltaDb); // clearly compressed
}

TEST_CASE("Limiter: output never exceeds the ceiling")
{
    Limiter lim;
    lim.prepare(48000.0);
    lim.setCeilingDb(-0.3f); // ceiling ~0.966

    // Hammer it well above the ceiling.
    auto in = th::sine(220.0, 48000.0, 16000, 4.0f);
    std::vector<float> L = in, R = in;
    lim.processStereo(L.data(), R.data(), static_cast<int>(L.size()));

    const float ceiling = std::pow(10.0f, -0.3f / 20.0f);
    REQUIRE(th::maxAbs(L) <= ceiling * 1.001f);
    REQUIRE(th::maxAbs(R) <= ceiling * 1.001f);
}

TEST_CASE("Limiter: leaves quiet signal essentially untouched")
{
    Limiter lim;
    lim.prepare(48000.0);
    lim.setCeilingDb(-0.3f);

    auto in = th::sine(220.0, 48000.0, 16000, 0.2f); // well below ceiling
    std::vector<float> L = in, R = in;
    lim.processStereo(L.data(), R.data(), static_cast<int>(L.size()));

    // Account for the lookahead delay by comparing tail RMS.
    REQUIRE(th::rmsTail(L) == Approx(th::rmsTail(in)).epsilon(0.02));
}
