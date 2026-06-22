#pragma once

// Hearing-test-based EQ personalization. A hearing test measures, per frequency,
// how much louder than a normal-hearing reference the listener needs a tone to
// just hear it (the elevated threshold, in dB). We turn that into a corrective
// parametric-EQ curve using audiology's half-gain rule (Lybarger): prescribe
// roughly half the measured loss as boost — full compensation is too bright and
// fatiguing. Boosts are capped and tiny losses ignored, so a normal-hearing
// result yields no bands (flat). Pure logic; no DSP/JUCE dependency.

#include <algorithm>
#include <vector>

#include "veyra/Config.h" // ParametricBand

namespace veyra {

struct HearingPoint {
    float freq = 1000.0f; // Hz
    float lossDb = 0.0f;  // elevated threshold vs normal hearing (>= 0)
};

// Build corrective bands from hearing-test points. fraction = half-gain rule by
// default; maxBoostDb caps any single band; minBoostDb drops negligible bands.
inline std::vector<ParametricBand>
personalizeFromHearingTest(const std::vector<HearingPoint>& points,
                           float fraction = 0.5f, float maxBoostDb = 12.0f,
                           float minBoostDb = 0.5f)
{
    std::vector<ParametricBand> bands;
    for (const auto& p : points)
    {
        if (p.freq <= 0.0f || p.lossDb <= 0.0f)
            continue;
        const float gain = std::min(p.lossDb * fraction, maxBoostDb);
        if (gain < minBoostDb)
            continue;
        ParametricBand b;
        b.enabled = true;
        b.type    = 0;      // peaking
        b.freq    = p.freq;
        b.gainDb  = gain;
        b.q       = 1.4f;
        bands.push_back(b);
    }
    std::sort(bands.begin(), bands.end(),
              [](const ParametricBand& a, const ParametricBand& b) { return a.freq < b.freq; });
    return bands;
}

} // namespace veyra
