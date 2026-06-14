#pragma once

// 10-band stereo graphic equaliser: one peaking biquad per ISO octave band,
// per channel. Band gains are smoothed and coefficients are refreshed at block
// rate (only when a band is actually moving), which keeps changes zipper-free
// without recomputing transcendentals every sample.

#include <array>

#include "chain/SmoothedValue.h"
#include "eq/Biquad.h"

namespace veyra::dsp {

class GraphicEq {
public:
    static constexpr int kNumBands = 10;

    // ISO-ish octave centres spanning the audible range.
    static constexpr std::array<double, kNumBands> kCentres = {
        31.25, 62.5, 125.0, 250.0, 500.0, 1000.0, 2000.0, 4000.0, 8000.0, 16000.0};

    void prepare(double sampleRate, int /*maxBlock*/ = 0) noexcept
    {
        sampleRate_ = sampleRate;
        for (int b = 0; b < kNumBands; ++b)
        {
            gain_[b].prepare(sampleRate);
            gain_[b].reset(0.0f);
            lastGainDb_[b] = 0.0f;
            const auto c = makePeaking(sampleRate_, kCentres[b], kQ, 0.0);
            left_[b].setCoeffs(c);
            right_[b].setCoeffs(c);
            left_[b].reset();
            right_[b].reset();
        }
    }

    // Gain in dB; the same value applies to both channels.
    void setBandGainDb(int band, float db) noexcept
    {
        if (band >= 0 && band < kNumBands)
            gain_[band].setTarget(db);
    }

    void processStereo(float* left, float* right, int numSamples) noexcept
    {
        // Block-rate coefficient update for any band still gliding.
        for (int b = 0; b < kNumBands; ++b)
        {
            const float g = gain_[b].skip(numSamples);
            if (std::fabs(g - lastGainDb_[b]) > 1.0e-4f)
            {
                lastGainDb_[b] = g;
                const auto c = makePeaking(sampleRate_, kCentres[b], kQ, g);
                left_[b].setCoeffs(c);
                right_[b].setCoeffs(c);
            }
        }

        for (int i = 0; i < numSamples; ++i)
        {
            float l = left[i];
            float r = right[i];
            for (int b = 0; b < kNumBands; ++b)
            {
                l = left_[b].process(l);
                r = right_[b].process(r);
            }
            left[i] = l;
            right[i] = r;
        }
    }

private:
    // Octave-spaced peaking filters overlap nicely around Q ~ 1.4.
    static constexpr double kQ = 1.41;

    double sampleRate_ = 48000.0;
    std::array<SmoothedValue, kNumBands> gain_;
    std::array<float, kNumBands> lastGainDb_{};
    std::array<Biquad, kNumBands> left_;
    std::array<Biquad, kNumBands> right_;
};

} // namespace veyra::dsp
