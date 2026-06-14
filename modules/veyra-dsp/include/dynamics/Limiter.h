#pragma once

// Lookahead brick-wall limiter with a hard mathematical guarantee that the
// output magnitude never exceeds the ceiling.
//
// The trick: delay the signal by the lookahead, and drive the gain from a
// sliding-window maximum of the link peak over the same window. The sample
// being output is the *oldest* in that window, so windowMax >= |out|, and
//   |out * (ceiling / windowMax)| <= ceiling
// always holds. Release is one-poled but clamped to never rise above that safe
// gain, so the guarantee survives smoothing. (True-peak oversampling is a
// later refinement; this limits sample peaks.)

#include <algorithm>
#include <array>
#include <cmath>

namespace veyra::dsp {

class Limiter {
public:
    static constexpr int kLookahead = 64;          // ~1.3 ms at 48 kHz
    static constexpr int kWindow = kLookahead + 1; // window includes the output sample

    void prepare(double sampleRate) noexcept
    {
        sampleRate_ = sampleRate;
        setReleaseMs(releaseMs_);
        reset();
    }

    void reset() noexcept
    {
        bufL_.fill(0.0f);
        bufR_.fill(0.0f);
        bufPeak_.fill(0.0f);
        pos_ = 0;
        gain_ = 1.0f;
    }

    void setCeilingDb(float db) noexcept { ceiling_ = std::pow(10.0f, db / 20.0f); }
    void setReleaseMs(float ms) noexcept
    {
        releaseMs_ = ms;
        const double t = (ms * 0.001) * sampleRate_;
        releaseCoeff_ = t > 0.0 ? static_cast<float>(std::exp(-1.0 / t)) : 0.0f;
    }

    int latencySamples() const noexcept { return kLookahead; }

    void processStereo(float* left, float* right, int numSamples) noexcept
    {
        for (int i = 0; i < numSamples; ++i)
        {
            const float peak = std::max(std::fabs(left[i]), std::fabs(right[i]));

            // Write current sample, then advance to the oldest slot.
            bufL_[pos_] = left[i];
            bufR_[pos_] = right[i];
            bufPeak_[pos_] = peak;
            pos_ = (pos_ + 1) % kWindow;

            const float outL = bufL_[pos_];
            const float outR = bufR_[pos_];

            float windowMax = 0.0f;
            for (int k = 0; k < kWindow; ++k)
                windowMax = std::max(windowMax, bufPeak_[k]);

            const float safeGain = windowMax > ceiling_ ? ceiling_ / windowMax : 1.0f;

            // Instant attack (more reduction); clamped release (never above safe).
            if (safeGain < gain_)
                gain_ = safeGain;
            else
                gain_ = safeGain + releaseCoeff_ * (gain_ - safeGain);

            left[i]  = outL * gain_;
            right[i] = outR * gain_;
        }
    }

private:
    double sampleRate_ = 48000.0;
    float ceiling_ = 0.966f; // ~ -0.3 dBFS
    float releaseMs_ = 50.0f;
    float releaseCoeff_ = 0.0f;

    std::array<float, kWindow> bufL_{};
    std::array<float, kWindow> bufR_{};
    std::array<float, kWindow> bufPeak_{};
    int pos_ = 0;
    float gain_ = 1.0f;
};

} // namespace veyra::dsp
