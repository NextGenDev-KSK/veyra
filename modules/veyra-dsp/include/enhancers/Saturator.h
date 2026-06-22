#pragma once

// Full-band saturation for warmth/glue. Three voicings:
//   0 Transparent — symmetric tanh (odd harmonics, clean).
//   1 Tape        — algebraic soft clip (gentle, symmetric).
//   2 Tube        — asymmetric (adds even harmonics, "warm").
// Each curve is normalised so a full-scale input maps to ~full scale, then mixed
// dry/wet by amount. A DC blocker removes the offset the asymmetric (tube) curve
// introduces. Header-only, RT-safe, allocation-free. amount 0..1 (0 = bypass).
//
// Optional 2x oversampling: the waveshaper is the chain's main alias source — a
// nonlinearity makes harmonics above Nyquist fold back into the audband. With
// oversampling on, only the *shaped* signal is run at 2x (linear-interpolated up,
// shaped, low-passed below Nyquist, decimated) so those harmonics are filtered
// out instead of aliasing; the dry path stays untouched.

#include <algorithm>
#include <cmath>

#include "eq/Biquad.h"

namespace veyra::dsp {

class Saturator {
public:
    void prepare(double sampleRate) noexcept
    {
        const double fs = sampleRate > 0 ? sampleRate : 48000.0;
        const double pi = 3.14159265358979323846;
        r_ = (float) std::exp(-2.0 * pi * 5.0 / fs); // DC blocker pole (~5 Hz)
        // Anti-alias / decimation low-pass, run in the 2x domain (fs2 = 2*fs),
        // cutoff just under the original Nyquist.
        const auto lp = makeLowPass(2.0 * fs, 0.45 * fs, 0.707);
        osLpfL_.setCoeffs(lp);
        osLpfR_.setCoeffs(lp);
        reset();
    }

    void reset() noexcept
    {
        x1L_ = y1L_ = x1R_ = y1R_ = 0.0f;
        prevL_ = prevR_ = 0.0f;
        osLpfL_.reset();
        osLpfR_.reset();
    }

    void setAmount(float a) noexcept { amount_ = std::clamp(a, 0.0f, 1.0f); }
    void setMode(int m) noexcept { mode_ = std::clamp(m, 0, 2); }
    void setOversample(bool on) noexcept { oversample_ = on; }

    void processStereo(float* left, float* right, int numSamples) noexcept
    {
        if (amount_ <= 0.001f)
            return;
        const float drive = 1.0f + amount_ * 2.0f; // 1..3
        const float mix = amount_;
        for (int i = 0; i < numSamples; ++i)
        {
            left[i]  = dcBlock(blend(left[i],  drive, mix, prevL_, osLpfL_), x1L_, y1L_);
            right[i] = dcBlock(blend(right[i], drive, mix, prevR_, osLpfR_), x1R_, y1R_);
        }
    }

private:
    // Dry/wet blend; the wet (shaped) part is optionally 2x oversampled.
    float blend(float x, float drive, float mix, float& prev, Biquad& lp) noexcept
    {
        float wet;
        if (oversample_)
        {
            const float sh0 = shape(0.5f * (prev + x), drive); // interpolated sub-sample
            const float sh1 = shape(x, drive);                 // on-grid sub-sample
            lp.process(sh0);          // advance the 2x-rate filter over both...
            wet = lp.process(sh1);    // ...then keep the on-grid one (decimate by 2)
            prev = x;
        }
        else
        {
            wet = shape(x, drive);
        }
        return (1.0f - mix) * x + mix * wet;
    }

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
    bool  oversample_ = false;
    float amount_ = 0.0f;
    float r_ = 0.9995f;
    float x1L_ = 0.0f, y1L_ = 0.0f, x1R_ = 0.0f, y1R_ = 0.0f;
    float prevL_ = 0.0f, prevR_ = 0.0f;
    Biquad osLpfL_, osLpfR_;
};

} // namespace veyra::dsp
