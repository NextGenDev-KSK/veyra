#pragma once

// Heuristic DSP classifier mapping a feature vector to a coarse sound class.
// Tuned for the competitive-gaming cues that matter: sharp broadband transients
// (gunshots), low-frequency thuds with onset (footsteps), and sustained midrange
// (voice). Thresholds scale with a 0..1 sensitivity. No model, fully testable.

#include <algorithm>

#include "tracker/TrackerTypes.h"

namespace veyra::dsp {

class SoundClassifier {
public:
    // 0 = conservative (only strong events), 1 = sensitive.
    void setSensitivity(float s) noexcept
    {
        const float v = std::clamp(s, 0.0f, 1.0f);
        noiseFloor_ = 0.06f - v * 0.055f;  // 0.06 .. 0.005
        fluxThresh_ = 0.12f - v * 0.09f;   // 0.12 .. 0.03
    }

    SoundClass classify(const AudioFeatures& f) const noexcept
    {
        if (f.rms < noiseFloor_)
            return SoundClass::None;

        const float total = f.lowEnergy + f.midEnergy + f.highEnergy + 1.0e-9f;
        const float lowFrac = f.lowEnergy / total;
        const float midFrac = f.midEnergy / total;
        const float highFrac = f.highEnergy / total;
        const bool sharp = f.flux > fluxThresh_;

        if (sharp && highFrac > 0.30f)
            return SoundClass::Gunshot;   // broadband sharp transient
        if (sharp && lowFrac > 0.45f)
            return SoundClass::Footstep;  // low-frequency thud with onset
        if (!sharp && midFrac > 0.45f)
            return SoundClass::Voice;     // sustained midrange
        return SoundClass::Other;
    }

private:
    float noiseFloor_ = 0.02f;
    float fluxThresh_ = 0.05f;
};

} // namespace veyra::dsp
