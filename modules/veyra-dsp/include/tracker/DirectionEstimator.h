#pragma once

// Estimates the horizontal direction of a sound from a stereo pair using the
// inter-channel level difference (ILD). Robust and cheap; phase/ITD refinement
// can layer on later. Azimuth is -90 (hard left) .. +90 (hard right).

namespace veyra::dsp {

struct DirectionEstimate {
    float azimuthDeg = 0.0f;
    float energy = 0.0f; // rmsL + rmsR (used as a confidence proxy)
};

class DirectionEstimator {
public:
    static DirectionEstimate fromStereoRms(float rmsL, float rmsR) noexcept
    {
        const float denom = rmsL + rmsR + 1.0e-9f;
        const float balance = (rmsR - rmsL) / denom; // -1 .. +1
        return {balance * 90.0f, rmsL + rmsR};
    }
};

} // namespace veyra::dsp
