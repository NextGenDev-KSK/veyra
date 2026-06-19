#include <catch2/catch.hpp>

#include <cmath>
#include <vector>

#include "spatial/VirtualSurround.h"

using namespace veyra::dsp;

namespace {
constexpr double kFs = 48000.0;
constexpr double kPi = 3.14159265358979323846;

float energy(const std::vector<float>& v, int from, int to)
{
    double e = 0.0;
    for (int i = from; i < to; ++i) e += (double) v[(size_t) i] * v[(size_t) i];
    return (float) e;
}
} // namespace

TEST_CASE("VirtualSurround: amount 0 is a bit-exact passthrough")
{
    VirtualSurround vs;
    vs.prepare(kFs);
    vs.setAmount(0.0f);

    std::vector<float> l = {0.2f, -0.4f, 0.6f, -0.1f}, r = {0.1f, 0.3f, -0.2f, 0.5f};
    const auto l0 = l, r0 = r;
    vs.processStereo(l.data(), r.data(), (int) l.size());
    CHECK(l == l0);
    CHECK(r == r0);
}

TEST_CASE("VirtualSurround: a left-only source stays left-dominant after virtualisation")
{
    VirtualSurround vs;
    vs.prepare(kFs);
    vs.setAmount(1.0f);

    const int n = 4096;
    std::vector<float> l((size_t) n), r((size_t) n, 0.0f);
    for (int i = 0; i < n; ++i)
        l[(size_t) i] = 0.5f * (float) std::sin(2.0 * kPi * 500.0 * i / kFs);

    vs.processStereo(l.data(), r.data(), n);

    // Skip the warm-up (latency + filter settle); measure the tail.
    const int from = n / 2;
    CHECK(energy(l, from, n) > energy(r, from, n)); // left speaker favours the left ear
    CHECK(energy(l, from, n) > 0.0f);               // not silent
}

TEST_CASE("VirtualSurround: a centred source renders symmetrically")
{
    VirtualSurround vs;
    vs.prepare(kFs);
    vs.setAmount(1.0f);

    const int n = 4096;
    std::vector<float> l((size_t) n), r((size_t) n);
    for (int i = 0; i < n; ++i)
    {
        const float s = 0.4f * (float) std::sin(2.0 * kPi * 600.0 * i / kFs);
        l[(size_t) i] = s;
        r[(size_t) i] = s; // identical -> centre
    }

    vs.processStereo(l.data(), r.data(), n);

    const int from = n / 2;
    CHECK(energy(l, from, n) == Approx(energy(r, from, n)).epsilon(0.05));
}
