#include <catch2/catch.hpp>

#include "veyra/RnnoiseDenoiser.h"

#include <cmath>
#include <cstdint>
#include <vector>

using namespace veyra;

TEST_CASE("RnnoiseDenoiser: builds, prepares, and is finite")
{
    RnnoiseDenoiser d;
    d.prepare(48000.0);

#if VEYRA_USE_RNNOISE
    REQUIRE(d.active()); // RNNoise compiled in + 48 kHz

    // ~1 s of stationary white noise — RNNoise should attenuate it (no speech).
    const int n = 48000;
    std::vector<float> x(n);
    uint32_t s = 0x12345u;
    auto rnd = [&]() { s ^= s << 13; s ^= s >> 17; s ^= s << 5; return (int32_t) s / 2147483648.0f; };
    for (auto& v : x) v = 0.1f * rnd();

    double inRms = 0.0;
    for (float v : x) inRms += (double) v * v;
    inRms = std::sqrt(inRms / n);

    d.processMono(x.data(), n);

    double outRms = 0.0;
    bool finite = true;
    const int half = n / 2; // skip priming/latency region
    for (int i = half; i < n; ++i) { if (!std::isfinite(x[i])) finite = false; outRms += (double) x[i] * x[i]; }
    outRms = std::sqrt(outRms / (n - half));

    CHECK(finite);
    CHECK(outRms < inRms); // stationary noise is suppressed
#else
    CHECK_FALSE(d.active()); // not built -> pass-through fallback
#endif
}

TEST_CASE("RnnoiseDenoiser: non-48k rate is an inactive pass-through")
{
    RnnoiseDenoiser d;
    d.prepare(44100.0);
    CHECK_FALSE(d.active());
    std::vector<float> x{0.2f, -0.2f, 0.1f};
    d.processMono(x.data(), 3);
    CHECK(x[0] == Approx(0.2f)); // untouched
}
