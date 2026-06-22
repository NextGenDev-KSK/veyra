#pragma once

// Measurement band analyzer for Sound Lab FR analysis. Runs the signal through a
// bank of log-spaced band-pass filters (octave centres, 31 Hz..16 kHz) and keeps
// a leaky-integrated RMS per band, giving a live magnitude readout — e.g. the
// measured response of a room/system when the captured stimulus is flat noise,
// or per-channel level matching. Header-only, RT-safe, allocation-free.

#include <array>
#include <cmath>

#include "eq/Biquad.h"

namespace veyra::dsp {

class MeasureBands {
public:
    static constexpr int kBands = 10;

    void prepare(double sampleRate) noexcept
    {
        sampleRate_ = sampleRate > 0 ? sampleRate : 48000.0;
        double fc = 31.25;
        for (int b = 0; b < kBands; ++b)
        {
            centre_[(size_t) b] = (float) fc;
            bp_[(size_t) b].setCoeffs(makeBandPass(sampleRate_, fc, 1.41));
            fc *= 2.0;
        }
        // ~120 ms integration time constant for a smooth but responsive readout.
        ema_ = (float) (1.0 - std::exp(-1.0 / (0.120 * sampleRate_)));
        reset();
    }

    void reset() noexcept
    {
        for (auto& f : bp_) f.reset();
        acc_.fill(0.0f);
    }

    void process(const float* x, int numSamples) noexcept
    {
        for (int i = 0; i < numSamples; ++i)
        {
            const float s = x[i];
            for (int b = 0; b < kBands; ++b)
            {
                const float y = bp_[(size_t) b].process(s);
                acc_[(size_t) b] += ema_ * (y * y - acc_[(size_t) b]);
            }
        }
    }

    float centreHz(int b) const noexcept { return centre_[(size_t) b]; }
    float levelLinear(int b) const noexcept { return std::sqrt(acc_[(size_t) b]); }
    float levelDb(int b) const noexcept
    {
        const float r = std::sqrt(acc_[(size_t) b]);
        return r > 1.0e-6f ? 20.0f * std::log10(r) : -120.0f;
    }

private:
    double sampleRate_ = 48000.0;
    float  ema_ = 0.01f;
    std::array<Biquad, kBands> bp_;
    std::array<float, kBands>  acc_{};
    std::array<float, kBands>  centre_{};
};

} // namespace veyra::dsp
