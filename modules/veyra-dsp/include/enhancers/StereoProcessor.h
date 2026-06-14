#pragma once

// Channel routing and stereo width. These sit at two different points in the
// chain (mono/balance early, the widener later), so they are exposed as two
// methods. All parameters are smoothed per sample — they are pure gain maths
// with no coefficients, so sample-rate smoothing is cheap and the smoothest
// option (a mono toggle becomes a ~5 ms crossfade, not a click).

#include "chain/SmoothedValue.h"

namespace veyra::dsp {

class StereoProcessor {
public:
    void prepare(double sampleRate) noexcept
    {
        balance_.prepare(sampleRate); balance_.reset(0.0f);
        width_.prepare(sampleRate);   width_.reset(1.0f);
        mono_.prepare(sampleRate);    mono_.reset(0.0f);
    }

    void setBalance(float balance) noexcept { balance_.setTarget(balance); } // -1..+1
    void setWidth(float width) noexcept     { width_.setTarget(width); }     // 0..2
    void setMono(bool mono) noexcept         { mono_.setTarget(mono ? 1.0f : 0.0f); }

    // Equal-power-ish constant-gain balance plus a smooth blend to mono.
    void applyMonoBalance(float* left, float* right, int numSamples) noexcept
    {
        for (int i = 0; i < numSamples; ++i)
        {
            const float bal = balance_.next();
            const float m = mono_.next();

            const float lg = bal <= 0.0f ? 1.0f : 1.0f - bal;
            const float rg = bal >= 0.0f ? 1.0f : 1.0f + bal;

            float l = left[i] * lg;
            float r = right[i] * rg;

            const float mid = 0.5f * (l + r);
            left[i]  = l + m * (mid - l);
            right[i] = r + m * (mid - r);
        }
    }

    // Mid-side widener: width 0 = mono, 1 = unchanged, 2 = doubled side.
    void applyWidth(float* left, float* right, int numSamples) noexcept
    {
        for (int i = 0; i < numSamples; ++i)
        {
            const float w = width_.next();
            const float mid = 0.5f * (left[i] + right[i]);
            const float side = 0.5f * (left[i] - right[i]) * w;
            left[i]  = mid + side;
            right[i] = mid - side;
        }
    }

private:
    SmoothedValue balance_, width_, mono_;
};

} // namespace veyra::dsp
