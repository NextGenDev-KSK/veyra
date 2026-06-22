#include <catch2/catch.hpp>

#include <cmath>
#include <vector>

#include "dynamics/Limiter.h"
#include "loudness/TruePeakMeter.h"

using namespace veyra::dsp;

namespace {
constexpr double kFs = 48000.0;
constexpr double kPi = 3.14159265358979323846;

// fs/4 sine at +45 deg: the samples sit at +/-A*0.707 (below the ceiling) but the
// continuous waveform peaks at A *between* samples -> a pure inter-sample peak.
std::vector<float> intersamplePeakSignal(int n, float amp)
{
    std::vector<float> s((size_t) n);
    for (int i = 0; i < n; ++i)
        s[(size_t) i] = amp * (float) std::sin(2.0 * kPi * (kFs / 4.0) * i / kFs + kPi / 4.0);
    return s;
}

float samplefabsMax(const std::vector<float>& v, int from)
{
    float m = 0.0f;
    for (int i = from; i < (int) v.size(); ++i) m = std::max(m, std::fabs(v[(size_t) i]));
    return m;
}

float truePeakOf(const std::vector<float>& v, int from)
{
    TruePeakMeter tp;
    tp.prepare(kFs);
    tp.processStereo(v.data() + from, v.data() + from, (int) v.size() - from);
    return tp.truePeakLinear();
}
} // namespace

TEST_CASE("Limiter: tames an inter-sample peak the sample peak hides")
{
    const float ceiling = std::pow(10.0f, -0.3f / 20.0f); // ~0.966
    const float amp = 1.3f; // sample peak ~0.919 (< ceiling), true peak ~1.3 (> ceiling)

    const int n = 4096;
    auto in = intersamplePeakSignal(n, amp);
    const float inSampleMax = samplefabsMax(in, n / 4);
    const float inTruePeak  = truePeakOf(in, n / 4);
    REQUIRE(inSampleMax < ceiling);          // sample limiter alone would do nothing
    REQUIRE(inTruePeak > ceiling + 0.05f);    // but it really overshoots between samples

    Limiter lim;
    lim.prepare(kFs);
    lim.setCeilingDb(-0.3f);
    auto l = in, r = in;
    lim.processStereo(l.data(), r.data(), n);

    const int settle = n / 2; // skip lookahead priming
    CHECK(samplefabsMax(l, settle) <= ceiling * 1.001f); // hard sample-peak guarantee
    const float outTruePeak = truePeakOf(l, settle);
    CHECK(outTruePeak < inTruePeak);          // inter-sample peak reduced
    CHECK(outTruePeak <= ceiling * 1.15f);    // pulled back near the ceiling
}

TEST_CASE("Limiter: leaves a signal already under the ceiling alone")
{
    Limiter lim;
    lim.prepare(kFs);
    lim.setCeilingDb(-0.3f);

    const int n = 2048;
    std::vector<float> l((size_t) n), r((size_t) n);
    for (int i = 0; i < n; ++i)
        l[(size_t) i] = r[(size_t) i] = 0.3f * (float) std::sin(2.0 * kPi * 1000.0 * i / kFs);
    const auto ref = l;
    lim.processStereo(l.data(), r.data(), n);

    // After the lookahead delay, output should equal the (delayed) input: gain 1.
    const int d = lim.latencySamples();
    for (int i = n / 2; i < n; ++i)
        CHECK(l[(size_t) i] == Approx(ref[(size_t) (i - d)]).margin(1e-4f));
}
