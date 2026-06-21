#pragma once

// The DSP chain's input parameters (Phase 3 output-chain subset). The APO will
// map its shared-memory parameter block onto this struct; the UI/service set
// these values. Plain data — no behaviour.

#include <array>

namespace veyra::dsp {

struct DspParameters {
    bool  bypass = false;
    bool  monoMode = false;
    float balance = 0.0f;        // -1 (L) .. +1 (R)

    std::array<float, 10> eqBandsDb{}; // EQ gains, dB (graphic bands / parametric bells)
    bool  parametricMode = false;      // false = graphic EQ, true = parametric EQ
    float bassBoostDb = 0.0f;          // low shelf @ 80 Hz
    float trebleDb = 0.0f;             // high shelf @ 8 kHz

    float compressionAmount = 0.0f;    // 0..1
    float reverbAmount = 0.0f;         // 0..1 ambience wet/dry
    float stereoWidth = 1.0f;          // 0 (mono) .. 2 (wide)
    float volumeGain = 1.0f;           // linear, 0..3 (up to 300%)
    float crossfeedAmount = 0.0f;      // 0..1 headphone crossfeed (spatial)
    float virtualizationAmount = 0.0f; // 0..1 HRTF headphone virtualisation
    float nightModeAmount = 0.0f;      // 0..1 late-night loudness compression
    bool  loudnessMatch = false;       // EBU R128 loudness-match (auto make-up)
    float loudnessTargetLufs = -14.0f; // target for loudness-match
    float limiterCeilingDb = -0.3f;    // true output ceiling
};

} // namespace veyra::dsp
