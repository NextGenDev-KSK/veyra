#pragma once

// Full-band saturation for warmth/glue. Three voicings:
//   0 Transparent — symmetric tanh (odd harmonics, clean).
//   1 Tape        — algebraic soft clip (gentle, symmetric).
//   2 Tube        — asymmetric (adds even harmonics, "warm").
// Each curve is normalised so a full-scale input maps to ~full scale, then mixed
// dry/wet by amount. A DC blocker removes the offset the asymmetric (tube) curve
// introduces. Header-only, RT-safe, allocation-free. amount 0..1 (0 = bypass).

#include <algorithm>
#include <cmath>

namespace veyra::dsp {

class Saturator {
public:
    void prepare(double sampleRate) noexcept
    {
        // DC blocker pole (~5 Hz high-pass).
        const double fc = 5.0;
        r_ = (float) std::exp(-2.0 * 3.14159265358979323846 * fc / (sampleRate > 0 ? sampleRate : 48000.0));
        reset();
    }

    void reset() noexcept { x1L_ = y1L_ = x1R_ = y1R_ = 0.0f; }

    void setAmount(float a) noexcept { amount_ = std::clamp(a, 0.0f, 1.0f); }
    void setMode(int m) noexcept { mode_ = std::clamp(m, 0, 2); }

    void processStereo(float* left, float* right, int numSamples) noexcept
    {
        if (amount_ <= 0.001f)
            return;
        const float drive = 1.0f + amount_ * 2.0f; // 1..3
        const float mix = amount_;
        for (int i = 0; i < numSamples; ++i)
        {
            float wl = (1.0f - mix) * left[i]  + mix * shape(left[i], drive);
            float wr = (1.0f - mix) * right[i] + mix * shape(right[i], drive);
            left[i]  = dcBlock(wl, x1L_, y1L_);
            right[i] = dcBlock(wr, x1R_, y1R_);
        }
    }

private:
    float shape(float x, float drive) const noexcept
    {
        switch (mode_)
        {
        case 1: // tape: algebraic soft clip
        {
            const float d = x * drive;
            const float norm = drive / (1.0f + drive);
            return (d / (1.0f + std::fabs(d))) / norm;
        }
        case 2: // tube: asymmetric (even harmonics)
        {
            constexpr float b = 0.25f;
            const float lo = std::tanh(b * drive);
            const float hi = std::tanh((1.0f + b) * drive);
            return (std::tanh((x + b) * drive) - lo) / (hi - lo);
        }
        default: // transparent: symmetric tanh
            return std::tanh(x * drive) / std::tanh(drive);
        }
    }

    float dcBlock(float x, float& x1, float& y1) noexcept
    {
        const float y = x - x1 + r_ * y1;
        x1 = x;
        y1 = y;
        return y;
    }

    int   mode_ = 0;
    float amount_ = 0.0f;
    float r_ = 0.9995f;
    float x1L_ = 0.0f, y1L_ = 0.0f, x1R_ = 0.0f, y1R_ = 0.0f;
};

} // namespace veyra::dsp
