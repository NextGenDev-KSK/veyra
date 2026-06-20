#include <catch2/catch.hpp>

#include <cmath>
#include <vector>

#include "loudness/LoudnessMeter.h"

using namespace veyra::dsp;

namespace {
constexpr double kFs = 48000.0;
constexpr double kPi = 3.14159265358979323846;

float measureIntegrated(double freq, float amp, double seconds)
{
    LoudnessMeter m;
    m.prepare(kFs);
    const int n = (int) (kFs * seconds);
    std::vector<float> l((size_t) n), r((size_t) n);
    for (int i = 0; i < n; ++i)
        l[(size_t) i] = r[(size_t) i] = amp * (float) std::sin(2.0 * kPi * freq * i / kFs);
    m.processStereo(l.data(), r.data(), n);
    return m.integratedLufs();
}
} // namespace

TEST_CASE("LoudnessMeter: silence reads the floor")
{
    LoudnessMeter m;
    m.prepare(kFs);
    std::vector<float> z((size_t) (kFs * 1.0), 0.0f);
    m.processStereo(z.data(), z.data(), (int) z.size());
    CHECK(m.integratedLufs() == Approx(LoudnessMeter::kSilence));
    CHECK(m.momentaryLufs() == Approx(LoudnessMeter::kSilence));
}

TEST_CASE("LoudnessMeter: a steady tone reads a finite loudness")
{
    const float lufs = measureIntegrated(1000.0, 0.5f, 2.0);
    CHECK(lufs > -40.0f);
    CHECK(lufs < 0.0f);
}

TEST_CASE("LoudnessMeter: +6 dB level -> +6 LU")
{
    const float quiet = measureIntegrated(1000.0, 0.25f, 2.0);
    const float loud  = measureIntegrated(1000.0, 0.50f, 2.0); // +6.02 dB
    CHECK((loud - quiet) == Approx(6.02f).margin(0.5f));
}

TEST_CASE("LoudnessMeter: K-weighting attenuates low frequencies")
{
    const float low  = measureIntegrated(50.0, 0.5f, 2.0);
    const float mid  = measureIntegrated(1000.0, 0.5f, 2.0);
    CHECK(mid > low + 2.0f); // 50 Hz is well below the K-weighting -> quieter
}

TEST_CASE("LoudnessMeter: momentary tracks the recent window")
{
    LoudnessMeter m;
    m.prepare(kFs);
    const int n = (int) (kFs * 0.6); // > 400 ms
    std::vector<float> l((size_t) n), r((size_t) n);
    for (int i = 0; i < n; ++i)
        l[(size_t) i] = r[(size_t) i] = 0.4f * (float) std::sin(2.0 * kPi * 1000.0 * i / kFs);
    m.processStereo(l.data(), r.data(), n);
    CHECK(m.momentaryLufs() > -40.0f);
    CHECK(m.momentaryLufs() < 0.0f);
}
