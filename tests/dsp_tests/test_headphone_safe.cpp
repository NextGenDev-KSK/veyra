#include <catch2/catch.hpp>

#include <cmath>
#include <vector>

#include "enhancers/HeadphoneSafe.h"

using namespace veyra::dsp;

namespace {
constexpr double kFs = 48000.0;
constexpr double kPi = 3.14159265358979323846;

float rms(HeadphoneSafe& h, double hz)
{
    const int n = 8192;
    std::vector<float> l(n), r(n);
    for (int i = 0; i < n; ++i)
        l[(size_t) i] = r[(size_t) i] = (float) std::sin(2.0 * kPi * hz * i / kFs);
    h.processStereo(l.data(), r.data(), n);
    double acc = 0.0;
    for (int i = n / 2; i < n; ++i) acc += (double) l[(size_t) i] * l[(size_t) i];
    return (float) std::sqrt(acc / (n / 2));
}
} // namespace

TEST_CASE("HeadphoneSafe tames the highs, leaves mids/lows")
{
    HeadphoneSafe h;
    h.prepare(kFs);
    h.setEnabled(true);

    CHECK(rms(h, 12000.0) < 0.7071f * 0.85f);          // upper band cut
    h.prepare(kFs); h.setEnabled(true);
    CHECK(rms(h, 300.0) == Approx(0.7071f).margin(0.03f)); // lows untouched
}

TEST_CASE("HeadphoneSafe disabled is an exact bypass")
{
    HeadphoneSafe h;
    h.prepare(kFs);
    h.setEnabled(false);
    std::vector<float> l = {0.3f, -0.6f, 0.9f, -0.2f}, r = l;
    const auto l0 = l;
    h.processStereo(l.data(), r.data(), (int) l.size());
    CHECK(l == l0);
}
