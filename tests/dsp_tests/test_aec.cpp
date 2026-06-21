#include <catch2/catch.hpp>

#include <cmath>
#include <vector>

#include "voice/AcousticEchoCanceller.h"

using namespace veyra::dsp;

namespace {
constexpr double kPi = 3.14159265358979323846;

float rms(const std::vector<float>& v, int from, int to)
{
    double acc = 0.0;
    for (int i = from; i < to; ++i) acc += (double) v[(size_t) i] * v[(size_t) i];
    const int n = to - from;
    return n > 0 ? (float) std::sqrt(acc / n) : 0.0f;
}
} // namespace

TEST_CASE("AEC: converges to cancel a delayed echo of the reference")
{
    AcousticEchoCanceller aec;
    aec.prepare(64);
    aec.setStepSize(0.5f);

    const int n = 20000;
    const int delay = 12;
    const float echoGain = 0.6f;

    // Far-end reference = deterministic noise; micSig = echo only (no micSig speech).
    std::vector<float> error(n, 0.0f);
    uint32_t s = 0x12345u;
    std::vector<float> refSig((size_t) n, 0.0f);
    for (int i = 0; i < n; ++i)
    {
        s ^= s << 13; s ^= s >> 17; s ^= s << 5;
        refSig[(size_t) i] = (float) (int32_t) s / 2147483648.0f * 0.5f;
    }
    for (int i = 0; i < n; ++i)
    {
        const float echo = (i >= delay) ? echoGain * refSig[(size_t) (i - delay)] : 0.0f;
        error[(size_t) i] = aec.processSample(echo, refSig[(size_t) i]);
    }

    const float before = rms(error, 0, 500);
    const float after  = rms(error, n - 2000, n);
    CHECK(after < before * 0.2f); // residual echo strongly reduced after adapting
}

TEST_CASE("AEC: preserves micSig-end speech while removing echo")
{
    AcousticEchoCanceller aec;
    aec.prepare(64);
    aec.setStepSize(0.5f);

    const int n = 40000;
    const int delay = 10;
    uint32_t s = 0xBEEFu;
    auto speech = [](int i) { return 0.3f * (float) std::sin(2.0 * kPi * 200.0 * i / 48000.0); };

    std::vector<float> refSig((size_t) n);
    for (int i = 0; i < n; ++i) { s ^= s << 13; s ^= s >> 17; s ^= s << 5; refSig[(size_t) i] = (float) (int32_t) s / 2147483648.0f * 0.4f; }

    double residErr = 0.0, echoEnergy = 0.0; int cnt = 0;
    for (int i = 0; i < n; ++i)
    {
        const float echo = (i >= delay) ? 0.5f * refSig[(size_t) (i - delay)] : 0.0f;
        const float micSig = speech(i) + echo;
        const float out = aec.processSample(micSig, refSig[(size_t) i]);
        if (i > n - 6000) { const float d = out - speech(i); residErr += (double) d * d;
                            echoEnergy += (double) echo * echo; ++cnt; }
    }
    // During continuous double-talk plain NLMS can't fully cancel, but it must
    // still meaningfully reduce the echo (residual << the raw echo level).
    const float resid   = (float) std::sqrt(residErr / cnt);
    const float echoRms = (float) std::sqrt(echoEnergy / cnt);
    CHECK(resid < echoRms * 0.75f);
}
