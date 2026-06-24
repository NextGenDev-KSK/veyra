#pragma once

// Per-frame audio feature extraction for the Sound Tracker. Splits a mono frame
// into low/high bands with biquads, derives a mid band, and tracks the onset
// flux (positive level change since the previous frame). Allocation-free.

#include <algorithm>
#include <cmath>

#include "eq/Biquad.h"
#include "tracker/TrackerTypes.h"

namespace veyra::dsp {

class FeatureExtractor {
public:
    void prepare(double sampleRate) noexcept
    {
        low_.setCoeffs(makeLowPass(sampleRate, 250.0, 0.707));
        high_.setCoeffs(makeHighPass(sampleRate, 2000.0, 0.707));
        reset();
    }

    void reset() noexcept
    {
        low_.reset();
        high_.reset();
        prevRms_ = 0.0f;
    }

    AudioFeatures process(const float* mono, int numSamples) noexcept
    {
        double sum = 0.0, lowSum = 0.0, highSum = 0.0;
        for (int i = 0; i < numSamples; ++i)
        {
            const float s = mono[i];
            sum += (double) s * s;
            const float lo = low_.process(s);
            const float hi = high_.process(s);
            lowSum += (double) lo * lo;
            highSum += (double) hi * hi;
        }
        const float inv = numSamples > 0 ? 1.0f / (float) numSamples : 0.0f;

        // Derive mid-band power in the power domain (not RMS domain) — RMS values
        // do not subtract linearly; only mean-square (power) values do.
        const float totalPow = (float) sum      * inv;
        const float lowPow   = (float) lowSum   * inv;
        const float highPow  = (float) highSum  * inv;
        const float midPow   = std::max(0.0f, totalPow - lowPow - highPow);

        AudioFeatures f;
        f.rms        = std::sqrt(totalPow);
        f.lowEnergy  = std::sqrt(lowPow);
        f.highEnergy = std::sqrt(highPow);
        f.midEnergy  = std::sqrt(midPow);
        f.flux = std::max(0.0f, f.rms - prevRms_);
        prevRms_ = f.rms;
        const float total = f.lowEnergy + f.midEnergy + f.highEnergy + 1.0e-9f;
        f.centroid = f.highEnergy / total;
        return f;
    }

private:
    Biquad low_, high_;
    float  prevRms_ = 0.0f;
};

} // namespace veyra::dsp
