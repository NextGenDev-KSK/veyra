#include <catch2/catch.hpp>

#include <cmath>
#include <vector>

#include "voice/VoiceChain.h"

using namespace veyra::dsp;

namespace {
constexpr double kFs = 48000.0;
constexpr double kPi = 3.14159265358979323846;

// Settled RMS of a 220 Hz sine of amplitude `amp` through the voice chain.
float procRms(VoiceChain& vc, float amp)
{
    const int n = 48000;
    std::vector<float> x(n);
    for (int i = 0; i < n; ++i)
        x[i] = amp * (float) std::sin(2.0 * kPi * 220.0 * i / kFs);
    vc.processMono(x.data(), n);
    double acc = 0.0;
    for (int i = n / 2; i < n; ++i) acc += (double) x[i] * x[i];
    return (float) std::sqrt(acc / (n / 2));
}

VoiceParams bare()
{
    VoiceParams p;
    p.enabled = true;
    p.noiseSuppression = 0.0f;
    p.compressionAmount = 0.0f;
    p.deEssAmount = 0.0f;
    p.presenceDb = 0.0f;
    return p;
}
} // namespace

TEST_CASE("AGC raises a quiet signal toward the target")
{
    VoiceParams p = bare();
    p.agc = true;
    VoiceChain vc;
    vc.prepare(kFs);
    vc.setParams(p);

    const float out = procRms(vc, 0.05f);
    CHECK(std::isfinite(out));
    CHECK(out > 0.05f * 0.707f * 1.5f); // clearly louder than the quiet input
    CHECK(out < 0.5f);                  // bounded — no runaway gain
}

TEST_CASE("AGC off leaves the level essentially unchanged")
{
    VoiceParams p = bare();
    p.agc = false;
    VoiceChain vc;
    vc.prepare(kFs);
    vc.setParams(p);

    const float in = 0.05f * 0.707f;
    CHECK(procRms(vc, 0.05f) == Approx(in).margin(0.012f));
}
