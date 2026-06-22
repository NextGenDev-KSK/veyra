#include <catch2/catch.hpp>

#include <algorithm>
#include <cmath>
#include <vector>

#include "enhancers/TransientShaper.h"

using namespace veyra::dsp;

namespace {
constexpr double kFs = 48000.0;
constexpr double kPi = 3.14159265358979323846;

// 200 ms silence, then a 400 ms 1 kHz tone (instant onset).
std::vector<float> burst()
{
    const int pre = (int) (0.2 * kFs);
    const int tone = (int) (0.4 * kFs);
    std::vector<float> v((size_t) (pre + tone), 0.0f);
    for (int i = 0; i < tone; ++i)
        v[(size_t) (pre + i)] = 0.5f * (float) std::sin(2.0 * kPi * 1000.0 * i / kFs);
    return v;
}

float maxAbs(const std::vector<float>& v, double t0, double t1)
{
    float m = 0.0f;
    for (int i = (int) (t0 * kFs); i < (int) (t1 * kFs) && i < (int) v.size(); ++i)
        m = std::max(m, std::fabs(v[(size_t) i]));
    return m;
}

// onset/sustain peak ratio after the shaper at `amount`.
float ratio(float amount)
{
    TransientShaper t;
    t.prepare(kFs);
    t.setAmount(amount);
    auto l = burst(), r = l;
    t.processStereo(l.data(), r.data(), (int) l.size());
    const float onset   = maxAbs(l, 0.20, 0.215); // first 15 ms after onset
    const float sustain = maxAbs(l, 0.50, 0.60);  // settled tail
    return onset / (sustain + 1.0e-6f);
}
} // namespace

TEST_CASE("TransientShaper emphasises the attack over the sustain")
{
    CHECK(ratio(0.0f) == Approx(1.0f).margin(0.1f)); // bypass: flat, no emphasis
    CHECK(ratio(0.8f) > 1.3f);                        // attack clearly boosted
}

TEST_CASE("TransientShaper at zero amount is an exact bypass")
{
    TransientShaper t;
    t.prepare(kFs);
    t.setAmount(0.0f);
    std::vector<float> l = {0.1f, -0.6f, 0.9f, -0.2f}, r = l;
    const auto l0 = l;
    t.processStereo(l.data(), r.data(), (int) l.size());
    CHECK(l == l0);
}
