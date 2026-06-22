#pragma once

// Headphone-safe listening mode (fatigue reduction). Tames the harsh upper band
// with a gentle high-shelf cut so long sessions are easier on the ears; pair with
// Loudness Match for full safe-listening. A toggle, not a knob. Header-only,
// RT-safe. When disabled it is an exact bypass.

#include "eq/Biquad.h"

namespace veyra::dsp {

class HeadphoneSafe {
public:
    void prepare(double sampleRate) noexcept
    {
        const auto sh = makeHighShelf(sampleRate > 0 ? sampleRate : 48000.0, 5000.0, 0.707, -3.5);
        shelfL_.setCoeffs(sh);
        shelfR_.setCoeffs(sh);
        reset();
    }

    void reset() noexcept { shelfL_.reset(); shelfR_.reset(); }

    void setEnabled(bool e) noexcept { enabled_ = e; }

    void processStereo(float* left, float* right, int numSamples) noexcept
    {
        if (!enabled_)
            return;
        for (int i = 0; i < numSamples; ++i)
        {
            left[i]  = shelfL_.process(left[i]);
            right[i] = shelfR_.process(right[i]);
        }
    }

private:
    Biquad shelfL_, shelfR_;
    bool   enabled_ = false;
};

} // namespace veyra::dsp
