#pragma once

// One-pole exponential parameter smoothing.
//
// Every continuously-variable parameter (EQ gain, volume, width, ...) is run
// through one of these so a value change glides to its target over a short time
// constant instead of jumping — that's what kills zipper noise. The math is a
// single multiply-add per sample and allocates nothing, so it is safe to call
// from the real-time audio thread.

#include <cmath>

namespace veyra::dsp {

class SmoothedValue {
public:
    // 'rampMs' is the time to cover ~63% of the remaining distance (the one-pole
    // time constant). 5 ms is the project default for audio-rate parameters.
    void prepare(double sampleRate, double rampMs = 5.0) noexcept
    {
        sampleRate_ = sampleRate > 0.0 ? sampleRate : 48000.0;
        setRampTime(rampMs);
    }

    void setRampTime(double rampMs) noexcept
    {
        const double tau = (rampMs * 0.001) * sampleRate_;
        coeff_ = tau > 0.0 ? std::exp(-1.0 / tau) : 0.0;
    }

    // Jump immediately to a value (e.g. on load/reset) — no glide.
    void reset(float value) noexcept
    {
        current_ = value;
        target_ = value;
    }

    void setTarget(float value) noexcept { target_ = value; }
    float target() const noexcept { return target_; }
    float current() const noexcept { return current_; }

    // Advance one sample and return the smoothed value.
    float next() noexcept
    {
        current_ = target_ + static_cast<float>(coeff_) * (current_ - target_);
        return current_;
    }

    // Advance 'n' samples at once (closed form) and return the result. Used by
    // block-rate consumers (e.g. EQ coefficient updates) so the glide still
    // follows the millisecond time constant regardless of block size.
    float skip(int n) noexcept
    {
        if (n <= 0)
            return current_;
        const float k = static_cast<float>(std::pow(coeff_, n));
        current_ = target_ + k * (current_ - target_);
        return current_;
    }

    // True once the smoother has effectively reached its target (lets callers
    // skip per-sample work when nothing is moving).
    bool isSmoothing(float epsilon = 1.0e-6f) const noexcept
    {
        return std::fabs(current_ - target_) > epsilon;
    }

private:
    double sampleRate_ = 48000.0;
    double coeff_ = 0.0;   // 0 => no smoothing (instant)
    float current_ = 0.0f;
    float target_ = 0.0f;
};

} // namespace veyra::dsp
