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

} // namespace veyra
