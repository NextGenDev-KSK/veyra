#include <catch2/catch.hpp>

#include <cmath>
#include <vector>

#include "loudness/LoudnessNormalizer.h"
#include "loudness/TruePeakMeter.h"

using namespace veyra::dsp;

namespace {
constexpr double kFs = 48000.0;
constexpr double kPi = 3.14159265358979323846;
} // namespace

TEST_CASE("TruePeakMeter: silence reads the floor")
{
    TruePeakMeter tp;
    tp.prepare(kFs);
    std::vector<float> z(256, 0.0f);
    tp.processStereo(z.data(), z.data(), 256);
    CHECK(tp.truePeakLinear() == Approx(0.0f).margin(1e-6f));
    CHECK(tp.truePeakDb() < -100.0f);
}

TEST_CASE("TruePeakMeter: catches an inter-sample peak above the sample peak")
{
    // fs/4 sine at +45 deg phase: samples sit at +/-0.707 but the true peak is 1.0
    // (the continuous waveform peaks exactly between samples).
    TruePeakMeter tp;
    tp.prepare(kFs);

    const int n = 2048;
    std::vector<float> s((size_t) n);
    float samplePeak = 0.0f;
    for (int i = 0; i < n; ++i)
    {
        s[(size_t) i] = (float) std::sin(2.0 * kPi * (kFs / 4.0) * i / kFs + kPi / 4.0);
        samplePeak = std::max(samplePeak, std::fabs(s[(size_t) i]));
    }
    tp.processStereo(s.data(), s.data(), n);

    CHECK(samplePeak == Approx(0.7071f).margin(0.01f));
    CHECK(tp.truePeakLinear() > 0.82f);                 // reconstructed overshoot
    CHECK(tp.truePeakLinear() > samplePeak + 0.1f);     // clearly above sample peak
}

TEST_CASE("LoudnessNormalizer: disabled is a passthrough")
{
    LoudnessNormalizer nz;
    nz.prepare(kFs);
    nz.setEnabled(false);
    std::vector<float> l = {0.1f, -0.2f, 0.3f}, r = l;
    const auto l0 = l;
    nz.processStereo(l.data(), r.data(), (int) l.size());
    CHECK(l == l0);
}

TEST_CASE("LoudnessNormalizer: boosts quiet, attenuates loud toward target")
{
    auto settle = [](float amp) -> float
    {
        LoudnessNormalizer nz;
        nz.prepare(kFs);
        nz.setEnabled(true);
        nz.setTargetLufs(-14.0f);

        const int block = (int) (kFs * 0.1); // 100 ms
        std::vector<float> l((size_t) block), r((size_t) block);
        for (int b = 0; b < 60; ++b) // ~6 s
        {
            for (int i = 0; i < block; ++i)
            {
                const int idx = b * block + i;
                l[(size_t) i] = r[(size_t) i] = amp * (float) std::sin(2.0 * kPi * 1000.0 * idx / kFs);
            }
            nz.processStereo(l.data(), r.data(), block);
        }
        return nz.currentGain();
    };

    CHECK(settle(0.05f) > 1.3f); // quiet program -> boosted
    CHECK(settle(0.80f) < 0.85f); // loud program -> attenuated
}
