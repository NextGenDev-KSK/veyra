#include <catch2/catch.hpp>

#include <cmath>
#include <vector>

#include "chain/DspChain.h"

using namespace veyra::dsp;

namespace {
constexpr double kFs = 48000.0;
constexpr double kPi = 3.14159265358979323846;

// RMS of a low-level 1 kHz sine through the chain (kept quiet so the limiter
// never engages and we isolate the EQ).
float rms1k(DspChain& chain)
{
    const int n = 8192;
    std::vector<float> l(n), r(n);
    for (int i = 0; i < n; ++i)
        l[i] = r[i] = 0.1f * (float) std::sin(2.0 * kPi * 1000.0 * i / kFs);
    chain.processStereo(l.data(), r.data(), n);
    double acc = 0.0;
    for (int i = n / 2; i < n; ++i) acc += (double) l[i] * l[i];
    return (float) std::sqrt(acc / (n / 2));
}
} // namespace

TEST_CASE("DspChain: parametric mode is wired and boosts the 1 kHz band")
{
    DspChain flat;
    flat.prepare(kFs, 8192);
    DspParameters pf;
    pf.parametricMode = true;            // flat parametric curve
    flat.setParameters(pf);

    DspChain boosted;
    boosted.prepare(kFs, 8192);
    DspParameters pb;
    pb.parametricMode = true;
    pb.eqBandsDb[5] = 12.0f;             // +12 dB at the 1 kHz band
    boosted.setParameters(pb);

    const float f = rms1k(flat);
    const float b = rms1k(boosted);
    CHECK(std::isfinite(b));
    CHECK(b > f * 1.4f);                 // the parametric bell clearly lifts 1 kHz
}
