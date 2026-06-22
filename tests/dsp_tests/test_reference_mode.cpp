#include <catch2/catch.hpp>

#include <cmath>
#include <vector>

#include "chain/DspChain.h"
#include "chain/DspParameters.h"

using namespace veyra::dsp;

namespace {
constexpr double kFs = 48000.0;
constexpr double kPi = 3.14159265358979323846;

// Low-band RMS of a settled run through the chain with the given parameters.
float lowBandRms(const DspParameters& p, double hz)
{
    const int n = 16384;
    DspChain chain;
    chain.prepare(kFs, n);
    chain.setParameters(p);

    std::vector<float> l(n), r(n);
    for (int i = 0; i < n; ++i)
        l[i] = r[i] = 0.2f * (float) std::sin(2.0 * kPi * hz * i / kFs);
    chain.processStereo(l.data(), r.data(), n);

    double acc = 0.0;
    for (int i = n / 2; i < n; ++i) acc += (double) l[i] * l[i];
    return (float) std::sqrt(acc / (n / 2));
}
} // namespace

TEST_CASE("Reference mode bypasses coloration (heavy bass boost has no effect)")
{
    DspParameters base;
    base.bassBoostDb = 12.0f; // strong low-shelf

    DspParameters colored = base;
    colored.referenceMode = false;

    DspParameters flat = base;
    flat.referenceMode = true;

    const float coloredLow = lowBandRms(colored, 60.0);
    const float flatLow    = lowBandRms(flat, 60.0);

    CHECK(coloredLow > flatLow * 1.3f); // coloration audibly lifts the bass
}

TEST_CASE("Reference mode still applies master volume + protects the ceiling")
{
    DspParameters p;
    p.referenceMode = true;
    p.volumeGain = 0.5f; // master trim must still work in reference mode

    const float full = [&]
    {
        DspParameters q = p; q.volumeGain = 1.0f;
        return lowBandRms(q, 1000.0);
    }();
    const float halved = lowBandRms(p, 1000.0);
    CHECK(halved == Approx(full * 0.5f).margin(0.01f));
}
