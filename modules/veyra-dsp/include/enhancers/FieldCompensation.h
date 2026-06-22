#pragma once

// Diffuse-field / free-field headphone target compensation. Headphones are
// voiced to a target curve; the main audible difference between the two standard
// targets is the ear's frontal ("free-field") gain around 3 kHz, which the
// diffuse-field target lacks. This stage lets the listener pick the presentation:
//   0 Off          — bypass.
//   1 Diffuse-field — relaxed, slightly less forward (gentle 3 kHz dip).
//   2 Free-field    — speaker-like / frontal (3 kHz ear-gain boost + a little air).
// Per-headphone-model correction is handled separately by AutoEQ (measured data);
// this is the broad target on top of it. Header-only, RT-safe. 0 = exact bypass.

#include <algorithm>

#include "eq/Biquad.h"

namespace veyra::dsp {

class FieldCompensation {
public:
    void prepare(double sampleRate) noexcept
    {
        sampleRate_ = sampleRate > 0 ? sampleRate : 48000.0;
        configure();
        reset();
    }

    void reset() noexcept { peakL_.reset(); peakR_.reset(); airL_.reset(); airR_.reset(); }

    void setMode(int m) noexcept
    {
        const int c = std::clamp(m, 0, 2);
        if (c != mode_) { mode_ = c; configure(); }
    }

    void processStereo(float* left, float* right, int numSamples) noexcept
    {
        if (mode_ == 0)
            return;
        for (int i = 0; i < numSamples; ++i)
        {
            left[i]  = airL_.process(peakL_.process(left[i]));
            right[i] = airR_.process(peakR_.process(right[i]));
        }
    }

private:
    void configure() noexcept
    {
        // 3 kHz presence peak + an optional high-shelf "air" for free-field.
        const double presenceDb = (mode_ == 2) ? 3.0 : (mode_ == 1 ? -1.5 : 0.0);
        const double airDb       = (mode_ == 2) ? 1.0 : 0.0;
        const auto pk = makePeaking(sampleRate_, 3000.0, 0.9, presenceDb);
        const auto sh = makeHighShelf(sampleRate_, 8000.0, 0.707, airDb);
        peakL_.setCoeffs(pk); peakR_.setCoeffs(pk);
        airL_.setCoeffs(sh);  airR_.setCoeffs(sh);
    }

    double sampleRate_ = 48000.0;
    int    mode_ = 0;
    Biquad peakL_, peakR_, airL_, airR_;
};

} // namespace veyra::dsp
