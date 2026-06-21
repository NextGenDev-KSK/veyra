#pragma once

// Maps the persisted Config onto the render DspChain's parameters. Used by the
// AudioBridge (and any other in-process DSP host) so the no-driver path applies
// exactly the same effects the APO path does.

#include <algorithm>

#include "chain/DspParameters.h"
#include "veyra/Config.h"

namespace veyra::service {

inline dsp::DspParameters dspParamsFromConfig(const Config& c)
{
    const auto& e = c.enhancement;
    dsp::DspParameters p;
    p.bypass   = !c.masterEnabled;
    p.monoMode = false;
    p.balance  = 0.0f;
    for (size_t i = 0; i < e.eqBandsDb.size() && i < p.eqBandsDb.size(); ++i)
        p.eqBandsDb[i] = e.eqBandsDb[i];
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
    p.limiterCeilingDb  = -0.3f;
    return p;
}

} // namespace veyra::service
