#pragma once

// Maps the persisted Config onto the render DspChain's parameters. Used by the
// AudioBridge (and any other in-process DSP host) so the no-driver path applies
// exactly the same effects the APO path does.

#include <algorithm>

#include "chain/DspParameters.h"
#include "eq/ParametricEq.h"

#include <array>
#include <utility>
#include "veyra/Config.h"

namespace veyra::service {

inline dsp::DspParameters dspParamsFromConfig(const Config& c)
{
    const auto& e = c.enhancement;
    dsp::DspParameters p;
    p.bypass   = !c.masterEnabled;
    p.referenceMode = c.referenceMode;
    p.monoMode = false;
    p.balance  = 0.0f;
    for (size_t i = 0; i < e.eqBandsDb.size() && i < p.eqBandsDb.size(); ++i)
        p.eqBandsDb[i] = e.eqBandsDb[i];
    p.parametricMode    = (e.eqMode == "parametric");
    p.bassBoostDb       = e.bassBoostDb;
    p.trebleDb          = e.trebleDb;
    p.compressionAmount = e.compressionAmount;
    p.reverbAmount      = e.reverbAmount;
    p.stereoWidth       = e.stereoWidth;
    // Effective output gain = master trim x the Volume Gain knob (clamped 0..3).
    p.volumeGain        = std::clamp(static_cast<float>(c.masterVolumeGain) * e.volumeGain, 0.0f, 3.0f);
    p.crossfeedAmount      = c.spatial.enabled ? c.spatial.crossfeed : 0.0f;
    p.virtualizationAmount = c.spatial.enabled ? c.spatial.virtualization : 0.0f;
    p.nightModeAmount      = c.loudness.nightModeAmount;
    p.loudnessMatch        = c.loudness.loudnessMatch;
    p.loudnessTargetLufs   = c.loudness.targetLufs;
    p.equalLoudness        = c.loudness.equalLoudness;
    p.limiterCeilingDb  = -0.3f;
    return p;
}

// Explicit parametric bands from config (empty -> count 0 -> DspChain derives the
// curve from the 10 band gains instead).
inline std::pair<std::array<dsp::EqBand, dsp::ParametricEq::kMaxBands>, int>
parametricBandsFromConfig(const Config& c)
{
    std::array<dsp::EqBand, dsp::ParametricEq::kMaxBands> out{};
    int n = 0;
    for (const auto& b : c.enhancement.parametricBands)
    {
        if (n >= dsp::ParametricEq::kMaxBands) break;
        dsp::EqBand e;
        e.enabled = b.enabled;
        e.type    = static_cast<dsp::EqBandType>(std::clamp(b.type, 0, 5));
        e.freq    = b.freq;
        e.gainDb  = b.gainDb;
        e.q       = b.q;
        out[(size_t) n++] = e;
    }
    return {out, n};
}

} // namespace veyra::service
