#include <catch2/catch.hpp>

#include <vector>

#include "enhancers/DelayLine.h"

using namespace veyra::dsp;

namespace {
constexpr double kFs = 48000.0;
}

TEST_CASE("DelayLine: zero delay is a passthrough")
{
    DelayLine d;
    d.prepare(kFs, 50.0f);
    d.setDelayMs(0.0f);
    CHECK(d.delaySamples() == 0);

    std::vector<float> l = {1.0f, 0.5f, -0.5f, 0.25f}, r = l;
    const auto l0 = l;
    d.processStereo(l.data(), r.data(), (int) l.size());
    CHECK(l == l0);
}

TEST_CASE("DelayLine: shifts the signal by the configured samples")
{
    DelayLine d;
    d.prepare(kFs, 50.0f);
    // 10 samples at 48 kHz.
    const int want = 10;
    d.setDelayMs(1000.0f * want / (float) kFs);
    REQUIRE(d.delaySamples() == want);

    const int n = 64;
    std::vector<float> l(n, 0.0f), r(n, 0.0f);
    l[0] = r[0] = 1.0f; // impulse at sample 0
    d.processStereo(l.data(), r.data(), n);

    for (int i = 0; i < n; ++i)
    {
        const float expected = (i == want) ? 1.0f : 0.0f;
        CHECK(l[(size_t) i] == Approx(expected).margin(1e-6f));
        CHECK(r[(size_t) i] == Approx(expected).margin(1e-6f));
    }
}

TEST_CASE("DelayLine: latency compensation aligns two routes")
{
    // Route A is the slow endpoint (20 ms); route B is fast (5 ms). Delaying B
    // by (20 - 5) ms makes both emit the impulse on the same output sample.
    const float maxLatencyMs = 20.0f, bLatencyMs = 5.0f;

    DelayLine b;
    b.prepare(kFs, 64.0f);
    b.setDelayMs(maxLatencyMs - bLatencyMs);

    const int n = 2048;
    std::vector<float> bl(n, 0.0f), br(n, 0.0f);
    bl[0] = br[0] = 1.0f;
    b.processStereo(bl.data(), br.data(), n);

    const int aImpulse = (int) std::lround(maxLatencyMs * 0.001 * kFs);   // endpoint A's own latency
    const int bImpulse = (int) std::lround(bLatencyMs * 0.001 * kFs) + b.delaySamples();
    CHECK(bImpulse == aImpulse);            // both land on the same sample
    CHECK(bl[(size_t) b.delaySamples()] == Approx(1.0f).margin(1e-6f));
}
