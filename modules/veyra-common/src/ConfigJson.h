#pragma once

// Internal (not installed) nlohmann (de)serialisation for the shared audio
// structs, so Config and Preset don't each duplicate the enhancement mapping.
// from_json is deliberately defensive (no exceptions, missing keys keep the
// destination's defaults) so callers can parse partial/old documents safely.

#include <nlohmann/json.hpp>

#include "veyra/Config.h"

namespace veyra {

inline void to_json(nlohmann::json& j, const EnhancementConfig& e)
{
    j = nlohmann::json{
        {"eq_bands_db",        e.eqBandsDb},
        {"eq_mode",            e.eqMode},
        {"bass_boost_db",      e.bassBoostDb},
        {"treble_db",          e.trebleDb},
        {"volume_gain",        e.volumeGain},
        {"stereo_width",       e.stereoWidth},
        {"compression_amount", e.compressionAmount},
        {"reverb_amount",      e.reverbAmount},
    };
}

inline void from_json(const nlohmann::json& j, EnhancementConfig& e)
{
    if (! j.is_object())
        return;
    if (const auto it = j.find("eq_bands_db");
        it != j.end() && it->is_array() && it->size() == e.eqBandsDb.size())
        for (size_t i = 0; i < e.eqBandsDb.size(); ++i)
            e.eqBandsDb[i] = (*it)[i].get<float>();
    e.eqMode            = j.value("eq_mode", e.eqMode);
    e.bassBoostDb       = j.value("bass_boost_db", e.bassBoostDb);
    e.trebleDb          = j.value("treble_db", e.trebleDb);
    e.volumeGain        = j.value("volume_gain", e.volumeGain);
    e.stereoWidth       = j.value("stereo_width", e.stereoWidth);
    e.compressionAmount = j.value("compression_amount", e.compressionAmount);
    e.reverbAmount      = j.value("reverb_amount", e.reverbAmount);
}

inline void to_json(nlohmann::json& j, const SpatialConfig& s)
{
    j = nlohmann::json{
        {"enabled",   s.enabled},
        {"crossfeed", s.crossfeed},
        {"mode",      s.mode},
    };
}

inline void from_json(const nlohmann::json& j, SpatialConfig& s)
{
    if (! j.is_object())
        return;
    s.enabled   = j.value("enabled", s.enabled);
    s.crossfeed = j.value("crossfeed", s.crossfeed);
    s.mode      = j.value("mode", s.mode);
}

inline void to_json(nlohmann::json& j, const VoiceConfig& v)
{
    j = nlohmann::json{
        {"enabled",            v.enabled},
        {"high_pass_hz",       v.highPassHz},
        {"noise_suppression",  v.noiseSuppression},
        {"compression_amount", v.compressionAmount},
        {"de_ess_amount",      v.deEssAmount},
        {"presence_db",        v.presenceDb},
        {"output_gain_db",     v.outputGainDb},
        {"side_tone_level",    v.sideToneLevel},
    };
}

inline void from_json(const nlohmann::json& j, VoiceConfig& v)
{
    if (! j.is_object())
        return;
    v.enabled           = j.value("enabled", v.enabled);
    v.highPassHz        = j.value("high_pass_hz", v.highPassHz);
    v.noiseSuppression  = j.value("noise_suppression", v.noiseSuppression);
    v.compressionAmount = j.value("compression_amount", v.compressionAmount);
    v.deEssAmount       = j.value("de_ess_amount", v.deEssAmount);
    v.presenceDb        = j.value("presence_db", v.presenceDb);
    v.outputGainDb      = j.value("output_gain_db", v.outputGainDb);
    v.sideToneLevel     = j.value("side_tone_level", v.sideToneLevel);
}

} // namespace veyra
