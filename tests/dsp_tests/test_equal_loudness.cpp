#include <catch2/catch.hpp>

#include <cmath>
#include <vector>

#include "loudness/EqualLoudness.h"

using namespace veyra::dsp;

namespace {
constexpr double kFs = 48000.0;
constexpr double kPi = 3.14159265358979323846;

constexpr int kN = 8192;

float rmsThrough(EqualLoudness& e, double hz)
{
    std::vector<float> l(kN), r(kN);
    for (int i = 0; i < kN; ++i) l[i] = r[i] = (float) std::sin(2.0 * kPi * hz * i / kFs);
    e.processStereo(l.data(), r.data(), kN);
    double acc = 0.0;
    for (int i = kN / 2; i < kN; ++i) acc += (double) l[i] * l[i];
    return (float) std::sqrt(acc / (kN / 2));
}

// RMS of the raw tone (no processing) — a non-integer cycle count over the
// window means this is not exactly 1/sqrt(2), so passthrough must match *this*.
float rawRms(double hz)
{
    double acc = 0.0;
    for (int i = kN / 2; i < kN; ++i)
    {
        const double s = std::sin(2.0 * kPi * hz * i / kFs);
        acc += s * s;
    }
    return (float) std::sqrt(acc / (kN / 2));
}
} // namespace

TEST_CASE("EqualLoudness: low volume lifts bass + treble, leaves mids")
{
    EqualLoudness e;
    e.prepare(kFs);
    e.setEnabled(true);
    e.setReference(0.2f); // well below reference -> compensation kicks in

    CHECK(rmsThrough(e, 60.0)    > rawRms(60.0) * 1.3f);   // bass boosted
    e.prepare(kFs); e.setEnabled(true); e.setReference(0.2f);
    CHECK(rmsThrough(e, 1000.0)  == Approx(rawRms(1000.0)).margin(0.06f)); // mids ~untouched
    e.prepare(kFs); e.setEnabled(true); e.setReference(0.2f);
    CHECK(rmsThrough(e, 13000.0) > rawRms(13000.0) * 1.1f); // treble lifted a little
}

TEST_CASE("EqualLoudness: at reference / disabled it's flat")
{
    EqualLoudness e;
    e.prepare(kFs);
    e.setEnabled(true);
    e.setReference(1.0f); // reference level -> no compensation -> passthrough
    CHECK(rmsThrough(e, 60.0) == Approx(rawRms(60.0)).margin(1e-4f));

    EqualLoudness off;
    off.prepare(kFs);
    off.setEnabled(false);
    off.setReference(0.1f);
    CHECK(rmsThrough(off, 60.0) == Approx(rawRms(60.0)).margin(1e-5f)); // exact passthrough
}
