#include <catch2/catch.hpp>

#include <chrono>
#include <cmath>
#include <cstdio>
#include <vector>

#include "chain/DspChain.h"
#include "voice/VoiceChain.h"

using namespace veyra::dsp;

namespace {
constexpr double kFs = 48000.0;

// Deterministic noise in [-amp, amp] (xorshift, no <random> cost).
struct Noise {
    uint32_t s = 0x12345678u;
    float next(float amp)
    {
        s ^= s << 13; s ^= s >> 17; s ^= s << 5;
        return amp * ((float) (int32_t) s / 2147483648.0f);
    }
};

DspParameters everythingOn()
{
    DspParameters p;
    p.bypass = false;
    for (int i = 0; i < 10; ++i) p.eqBandsDb[(size_t) i] = (i % 2 ? -4.0f : 5.0f);
    p.bassBoostDb = 6.0f;
    p.trebleDb = 6.0f;
    p.compressionAmount = 0.5f;
    p.stereoWidth = 1.5f;
    p.volumeGain = 3.0f;          // push hard so the limiter must work
    p.crossfeedAmount = 0.7f;
    p.virtualizationAmount = 0.7f; // HRTF convolution active
    p.nightModeAmount = 0.5f;
    p.limiterCeilingDb = -0.3f;
    return p;
}
} // namespace

TEST_CASE("Perf: render chain runs far faster than real time")
{
    DspChain chain;
    const int block = 512;
    chain.prepare(kFs, block);
    chain.setParameters(everythingOn());

    const double seconds = 10.0;
    const int blocks = (int) (kFs * seconds / block);
    std::vector<float> l((size_t) block), r((size_t) block);
    Noise ng;

    const auto t0 = std::chrono::steady_clock::now();
    for (int b = 0; b < blocks; ++b)
    {
        for (int i = 0; i < block; ++i) { l[(size_t) i] = ng.next(0.5f); r[(size_t) i] = ng.next(0.5f); }
        chain.processStereo(l.data(), r.data(), block);
    }
    const auto t1 = std::chrono::steady_clock::now();

    const double elapsed = std::chrono::duration<double>(t1 - t0).count();
    const double rtFactor = seconds / elapsed;
    std::printf("[perf] DspChain (all effects): %.1fx real-time (%.0f ms for %.0f s)\n",
                rtFactor, elapsed * 1000.0, seconds);

    // Conservative guard (CI runners vary); real numbers are far higher.
    CHECK(rtFactor > 5.0);
}

TEST_CASE("Perf: voice chain runs far faster than real time")
{
    VoiceChain vc;
    vc.prepare(kFs);
    vc.setParams(VoiceParams{}); // defaults: HPF + NS + comp + de-ess + presence

    const int block = 480;
    const double seconds = 10.0;
    const int blocks = (int) (kFs * seconds / block);
    std::vector<float> mono((size_t) block);
    Noise ng;

    const auto t0 = std::chrono::steady_clock::now();
    for (int b = 0; b < blocks; ++b)
    {
        for (int i = 0; i < block; ++i) mono[(size_t) i] = ng.next(0.3f);
        vc.processMono(mono.data(), block);
    }
    const auto t1 = std::chrono::steady_clock::now();

    const double rtFactor = seconds / std::chrono::duration<double>(t1 - t0).count();
    std::printf("[perf] VoiceChain: %.1fx real-time\n", rtFactor);
    CHECK(rtFactor > 5.0);
}

TEST_CASE("Soak: full chain stays finite and within the ceiling under abuse")
{
    DspChain chain;
    const int block = 256;
    chain.prepare(kFs, block);
    chain.setParameters(everythingOn());

    const double ceilingLin = std::pow(10.0, -0.3 / 20.0) + 0.03; // ceiling + tolerance
    std::vector<float> l((size_t) block), r((size_t) block);
    Noise ng;

    bool allFinite = true;
    float peak = 0.0f;
    const int blocks = (int) (kFs * 30.0 / block); // 30 s of full-scale noise
    for (int b = 0; b < blocks; ++b)
    {
        for (int i = 0; i < block; ++i) { l[(size_t) i] = ng.next(0.99f); r[(size_t) i] = ng.next(0.99f); }
        chain.processStereo(l.data(), r.data(), block);
        for (int i = 0; i < block; ++i)
        {
            if (!std::isfinite(l[(size_t) i]) || !std::isfinite(r[(size_t) i])) allFinite = false;
            peak = std::max(peak, std::max(std::fabs(l[(size_t) i]), std::fabs(r[(size_t) i])));
        }
    }

    CHECK(allFinite);                 // no NaN/inf ever
    CHECK(peak <= (float) ceilingLin); // true-peak limiter holds the ceiling
    std::printf("[perf] soak peak = %.4f (ceiling ~%.4f)\n", peak, ceilingLin);
}
