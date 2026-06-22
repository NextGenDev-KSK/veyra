#pragma once

// Volume-dependent equal-loudness compensation (ISO-226-inspired). The ear hears
// less bass and (a little) less treble as level drops, so as the listening
// volume goes below reference we gently lift a low shelf + high shelf to keep the
// tonal balance natural. referenceGain is the output volume used as a level proxy
// (1.0 = reference -> no compensation; quieter -> more). Header-only, RT-safe.

#include <algorithm>
#include <cmath>

#include "eq/Biquad.h"

namespace veyra::dsp {

class EqualLoudness {
public:
    void prepare(double sampleRate) noexcept
    {
        sampleRate_ = sampleRate > 0.0 ? sampleRate : 48000.0;
        reset();
        refresh();
    }

    void reset() noexcept
    {
        lowL_.reset(); lowR_.reset(); highL_.reset(); highR_.reset();
    }

    void setEnabled(bool e) noexcept { enabled_ = e; }

    void setReference(float volumeGain) noexcept
    {
        const float v = std::clamp(volumeGain, 0.0f, 1.5f);
        const float boost = std::clamp((1.0f - v) * 10.0f, 0.0f, 10.0f); // dB below ref
        if (std::fabs(boost - boostDb_) > 0.1f)
        {
            boostDb_ = boost;
            refresh();
        }
    }

    void processStereo(float* left, float* right, int numSamples) noexcept
    {
        if (!enabled_ || boostDb_ <= 0.05f)
            return;
        for (int i = 0; i < numSamples; ++i)
        {
            left[i]  = highL_.process(lowL_.process(left[i]));
            right[i] = highR_.process(lowR_.process(right[i]));
        }
    }

private:
    void refresh() noexcept
    {
        const auto lo = makeLowShelf(sampleRate_, 120.0, 0.70, boostDb_);
        const auto hi = makeHighShelf(sampleRate_, 10000.0, 0.70, boostDb_ * 0.6f);
        lowL_.setCoeffs(lo);  lowR_.setCoeffs(lo);
        highL_.setCoeffs(hi); highR_.setCoeffs(hi);
    }

    double sampleRate_ = 48000.0;
    bool   enabled_ = false;
    float  boostDb_ = 0.0f;
    Biquad lowL_, lowR_, highL_, highR_;
};

} // namespace veyra::dsp
