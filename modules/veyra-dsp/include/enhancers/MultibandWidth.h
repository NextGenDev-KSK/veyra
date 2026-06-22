#pragma once

// Multiband stereo width. Splits the signal at a low crossover (~300 Hz) with a
// complementary low-pass / high-pass pair so each band is spectrally clean (a
// plain input-minus-low split leaks low frequencies into the high band because
// of the low-pass phase lag, which would then get *widened* instead of mono'd).
// Collapses the low band toward mono (tidy, punchy bass) and widens the high
// band, both scaled by amount. Header-only, RT-safe. amount 0..1 (0 = bypass).

#include <algorithm>
#include <cmath>

#include "eq/Biquad.h"

namespace veyra::dsp {

class MultibandWidth {
public:
    void prepare(double sampleRate) noexcept
    {
        const double fs = sampleRate > 0 ? sampleRate : 48000.0;
        const auto lp = makeLowPass(fs, kSplitHz, 0.707);
        const auto hp = makeHighPass(fs, kSplitHz, 0.707);
        lpL_.setCoeffs(lp); lpR_.setCoeffs(lp);
        hpL_.setCoeffs(hp); hpR_.setCoeffs(hp);
        reset();
    }

    void reset() noexcept { lpL_.reset(); lpR_.reset(); hpL_.reset(); hpR_.reset(); }

    void setAmount(float a) noexcept { amount_ = std::clamp(a, 0.0f, 1.0f); }

    void processStereo(float* left, float* right, int numSamples) noexcept
    {
        if (amount_ <= 0.001f)
            return;
        const float mono  = amount_;        // 0..1 low-band mono blend
        const float width = 1.0f + amount_; // 1..2 high-band widening
        for (int i = 0; i < numSamples; ++i)
        {
            const float lowL = lpL_.process(left[i]);
            const float lowR = lpR_.process(right[i]);
            const float hiL  = hpL_.process(left[i]);
            const float hiR  = hpR_.process(right[i]);

            const float lm = (lowL + lowR) * 0.5f;
            const float lL = lowL + (lm - lowL) * mono;
            const float lR = lowR + (lm - lowR) * mono;

            const float mid  = (hiL + hiR) * 0.5f;
            const float side = (hiL - hiR) * 0.5f * width;

            left[i]  = lL + (mid + side);
            right[i] = lR + (mid - side);
        }
    }

private:
    static constexpr double kSplitHz = 300.0;
    Biquad lpL_, lpR_, hpL_, hpR_;
    float  amount_ = 0.0f;
};

} // namespace veyra::dsp
