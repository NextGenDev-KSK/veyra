#include <catch2/catch.hpp>

#include <cmath>
#include <vector>

#include "loudness/EqualLoudness.h"

using namespace veyra::dsp;

namespace {
constexpr double kFs = 48000.0;
constexpr double kPi = 3.14159265358979323846;

float rmsThrough(EqualLoudness& e, double hz)
{
    const int n = 8192;
    std::vector<float> l(n), r(n);
    for (int i = 0; i < n; ++i) l[i] = r[i] = (float) std::sin(2.0 * kPi * hz * i / kFs);
    e.processStereo(l.data(), r.data(), n);
    double acc = 0.0;
    for (int i = n / 2; i < n; ++i) acc += (double) l[i] * l[i];
    return (float) std::sqrt(acc / (n / 2));
}
} // namespace

TEST_CASE("EqualLoudness: low volume lifts bass + treble, leaves mids")
{
    EqualLoudness e;
    e.prepare(kFs);
    e.setEnabled(true);
    e.setReference(0.2f); // well below reference -> compensation kicks in

    const float ref = 0.7071f; // unit-sine RMS
    CHECK(rmsThrough(e, 60.0)    > ref * 1.3f);  // bass boosted
    e.prepare(kFs); e.setEnabled(true); e.setReference(0.2f);
    CHECK(rmsThrough(e, 1000.0)  == Approx(ref).margin(0.06f)); // mids ~untouched
    e.prepare(kFs); e.setEnabled(true); e.setReference(0.2f);
    CHECK(rmsThrough(e, 13000.0) > ref * 1.1f);  // treble lifted a little
}

TEST_CASE("EqualLoudness: at reference / disabled it's flat")
{
    EqualLoudness e;
    e.prepare(kFs);
    e.setEnabled(true);
    e.setReference(1.0f); // reference level -> no compensation
    CHECK(rmsThrough(e, 60.0) == Approx(0.7071f).margin(0.02f));

    EqualLoudness off;
    off.prepare(kFs);
    off.setEnabled(false);
    off.setReference(0.1f);
    CHECK(rmsThrough(off, 60.0) == Approx(0.7071f).margin(0.001f)); // bit-exact passthrough
}
