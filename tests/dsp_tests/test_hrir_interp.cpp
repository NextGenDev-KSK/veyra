#include <catch2/catch.hpp>

#include <vector>

#include "spatial/HrirInterp.h"

using namespace veyra::dsp;

namespace {
int peakIndex(const std::vector<float>& v)
{
    int best = 0;
    float m = 0.0f;
    for (int i = 0; i < (int) v.size(); ++i)
        if (std::fabs(v[(size_t) i]) > m) { m = std::fabs(v[(size_t) i]); best = i; }
    return best;
}
} // namespace

TEST_CASE("hrirInterpAligned: two delayed impulses interpolate to one coherent impulse")
{
    std::vector<float> a(64, 0.0f), b(64, 0.0f);
    a[5] = 1.0f;   // direct arrival at sample 5
    b[15] = 1.0f;  // ...and at sample 15 (a different ITD)

    std::vector<float> out;
    hrirInterpAligned(a, b, 0.5f, out);

    // ITD-aware: a single impulse at the *interpolated* delay (~10), full height —
    // not two half-height images (which naive crossfade would give).
    CHECK(peakIndex(out) == 10);
    CHECK(out[10] == Approx(1.0f).margin(0.05f));
    CHECK(std::fabs(out[5])  < 0.1f); // no ghost at a's original delay
    CHECK(std::fabs(out[15]) < 0.1f); // no ghost at b's original delay
}

TEST_CASE("hrirInterpAligned: endpoints reduce to the source IR")
{
    std::vector<float> a(32, 0.0f), b(32, 0.0f);
    a[4] = 1.0f; a[5] = -0.5f;
    b[12] = 0.8f;

    std::vector<float> out;
    hrirInterpAligned(a, b, 0.0f, out);
    CHECK(out == a);              // t=0 -> exactly a (measured-azimuth query)

    hrirInterpAligned(a, b, 1.0f, out);
    CHECK(out == b);              // t=1 -> exactly b
}
