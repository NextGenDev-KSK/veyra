#pragma once

// Transient shaper: emphasises attacks for extra punch/detail. Tracks a fast and
// a slow envelope of the link signal; when the fast envelope leads the slow one
// (an onset), it applies a proportional gain boost that fades as the slow
// envelope catches up — so sustained material is left alone. Header-only,
// RT-safe, allocation-free. amount 0..1 (0 = bypass).

#include <algorithm>
#include <cmath>

namespace veyra::dsp {

class TransientShaper {
public:
    void prepare(double sampleRate) noexcept
    {
        sampleRate_ = sampleRate > 0.0 ? sampleRate : 48000.0;
        aFast_ = coeff(0.003); // 3 ms
        aSlow_ = coeff(0.080); // 80 ms
        reset();
    }

    void reset() noexcept { fast_ = slow_ = 0.0f; }

    void setAmount(float a) noexcept { amount_ = std::clamp(a, 0.0f, 1.0f); }

    void processStereo(float* left, float* right, int numSamples) noexcept
    {
        if (amount_ <= 0.001f)
            return;
        for (int i = 0; i < numSamples; ++i)
        {
            const float x = std::max(std::fabs(left[i]), std::fabs(right[i]));
            fast_ += aFast_ * (x - fast_);
            slow_ += aSlow_ * (x - slow_);
            const float diff = fast_ - slow_;
            const float boost = diff > 0.0f ? std::min(1.0f, diff / (slow_ + 1.0e-4f)) : 0.0f;
            const float g = 1.0f + amount_ * 1.5f * boost; // up to ~+3.5 dB of attack
            left[i]  *= g;
            right[i] *= g;
        }
    }

private:
    float coeff(double tau) const noexcept
    {
        return (float) (1.0 - std::exp(-1.0 / (tau * sampleRate_)));
    }

    double sampleRate_ = 48000.0;
    float  aFast_ = 0.0f, aSlow_ = 0.0f;
    float  fast_ = 0.0f, slow_ = 0.0f;
    float  amount_ = 0.0f;
};

} // namespace veyra::dsp
