#pragma once

// Harmonic exciter ("air"/presence). Splits off the high band, soft-saturates it
// with tanh to synthesise upper harmonics, and blends that back proportionally.
// tanh is an odd nonlinearity, so it adds (mostly) odd harmonics without DC.
// Header-only, RT-safe, allocation-free. amount 0..1 (0 = transparent bypass).

#include <algorithm>
#include <cmath>

#include "eq/Biquad.h"

namespace veyra::dsp {

class Exciter {
public:
    void prepare(double sampleRate) noexcept
    {
        sampleRate_ = sampleRate > 0.0 ? sampleRate : 48000.0;
        const auto hp = makeHighPass(sampleRate_, kSplitHz, 0.707);
        hpL_.setCoeffs(hp);
        hpR_.setCoeffs(hp);
        reset();
    }

    void reset() noexcept { hpL_.reset(); hpR_.reset(); }

    void setAmount(float a) noexcept { amount_ = std::clamp(a, 0.0f, 1.0f); }

    void processStereo(float* left, float* right, int numSamples) noexcept
    {
        if (amount_ <= 0.001f)
            return;
        const float drive = 1.0f + amount_ * 4.0f; // 1..5
        const float mix   = amount_ * 0.5f;        // up to 50% harmonic blend
        const float comp  = 1.0f / drive;          // keep unity-ish level pre-mix
        for (int i = 0; i < numSamples; ++i)
        {
            const float hl = hpL_.process(left[i]);
            const float hr = hpR_.process(right[i]);
            left[i]  += mix * (std::tanh(hl * drive) * comp);
            right[i] += mix * (std::tanh(hr * drive) * comp);
        }
    }

private:
    static constexpr double kSplitHz = 3000.0; // high band fed to the saturator

    double sampleRate_ = 48000.0;
    float  amount_ = 0.0f;
    Biquad hpL_, hpR_;
};

} // namespace veyra::dsp
