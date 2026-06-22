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
        {"exciter_amount",     e.exciterAmount},
        {"saturation_amount",  e.saturationAmount},
        {"saturation_mode",    e.saturationMode},
        {"multiband_width",    e.multibandWidth},
    };
    auto bands = nlohmann::json::array();
    for (const auto& b : e.parametricBands)
        bands.push_back({{"enabled", b.enabled}, {"type", b.type},
                         {"freq", b.freq}, {"gain_db", b.gainDb}, {"q", b.q}});
    j["parametric_bands"] = bands;
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
    e.exciterAmount     = j.value("exciter_amount", e.exciterAmount);
    e.saturationAmount  = j.value("saturation_amount", e.saturationAmount);
    e.saturationMode    = j.value("saturation_mode", e.saturationMode);
    e.multibandWidth    = j.value("multiband_width", e.multibandWidth);
    e.parametricBands.clear();
    if (const auto it = j.find("parametric_bands"); it != j.end() && it->is_array())
        for (const auto& b : *it)
        {
            if (!b.is_object()) continue;
            ParametricBand pb;
            pb.enabled = b.value("enabled", true);
            pb.type    = b.value("type", 0);
            pb.freq    = b.value("freq", 1000.0f);
            pb.gainDb  = b.value("gain_db", 0.0f);
            pb.q       = b.value("q", 1.0f);
            e.parametricBands.push_back(pb);
        }
}

inline void to_json(nlohmann::json& j, const SpatialConfig& s)
{
    j = nlohmann::json{
        {"enabled",        s.enabled},
        {"crossfeed",      s.crossfeed},
        {"virtualization", s.virtualization},
        {"mode",           s.mode},
    };
}

inline void from_json(const nlohmann::json& j, SpatialConfig& s)
{
    if (! j.is_object())
        return;
    s.enabled        = j.value("enabled", s.enabled);
    s.crossfeed      = j.value("crossfeed", s.crossfeed);
    s.virtualization = j.value("virtualization", s.virtualization);
    s.mode           = j.value("mode", s.mode);
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
        {"noise_gate",         v.noiseGate},
        {"echo_cancel",        v.echoCancel},
        {"agc",                v.agc},
        {"profile",            v.profile},
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
    v.noiseGate         = j.value("noise_gate", v.noiseGate);
    v.echoCancel        = j.value("echo_cancel", v.echoCancel);
    v.agc               = j.value("agc", v.agc);
    v.profile           = j.value("profile", v.profile);
}

inline void to_json(nlohmann::json& j, const GamerModeConfig& g)
{
    j = nlohmann::json{
        {"enabled",     g.enabled},
        {"sensitivity", g.sensitivity},
        {"radar_mode",  g.radarMode},
        {"footsteps",   g.footsteps},
        {"gunshots",    g.gunshots},
        {"voices",      g.voices},
    };
}

inline void from_json(const nlohmann::json& j, GamerModeConfig& g)
{
    if (! j.is_object())
        return;
    g.enabled     = j.value("enabled", g.enabled);
    g.sensitivity = j.value("sensitivity", g.sensitivity);
    g.radarMode   = j.value("radar_mode", g.radarMode);
    g.footsteps   = j.value("footsteps", g.footsteps);
    g.gunshots    = j.value("gunshots", g.gunshots);
    g.voices      = j.value("voices", g.voices);
}

inline void to_json(nlohmann::json& j, const LoudnessConfig& l)
{
    j = nlohmann::json{
        {"night_mode_amount",   l.nightModeAmount},
        {"sleep_timer_enabled", l.sleepTimerEnabled},
        {"sleep_timer_minutes", l.sleepTimerMinutes},
        {"sleep_fade_seconds",  l.sleepFadeSeconds},
        {"loudness_match",      l.loudnessMatch},
        {"target_lufs",         l.targetLufs},
        {"equal_loudness",      l.equalLoudness},
    };
}

inline void from_json(const nlohmann::json& j, LoudnessConfig& l)
{
    if (! j.is_object())
        return;
    l.nightModeAmount   = j.value("night_mode_amount", l.nightModeAmount);
    l.sleepTimerEnabled = j.value("sleep_timer_enabled", l.sleepTimerEnabled);
    l.sleepTimerMinutes = j.value("sleep_timer_minutes", l.sleepTimerMinutes);
    l.sleepFadeSeconds  = j.value("sleep_fade_seconds", l.sleepFadeSeconds);
    l.loudnessMatch     = j.value("loudness_match", l.loudnessMatch);
    l.targetLufs        = j.value("target_lufs", l.targetLufs);
    l.equalLoudness     = j.value("equal_loudness", l.equalLoudness);
}

inline void to_json(nlohmann::json& j, const OutputRoute& r)
{
    j = nlohmann::json{
        {"device_id", r.deviceId},
        {"name",      r.name},
        {"enabled",   r.enabled},
        {"gain_db",   r.gainDb},
        {"delay_ms",  r.delayMs},
        {"primary",   r.primary},
    };
}

inline void from_json(const nlohmann::json& j, OutputRoute& r)
{
    if (! j.is_object())
        return;
    r.deviceId = j.value("device_id", r.deviceId);
    r.name     = j.value("name", r.name);
    r.enabled  = j.value("enabled", r.enabled);
    r.gainDb   = j.value("gain_db", r.gainDb);
    r.delayMs  = j.value("delay_ms", r.delayMs);
    r.primary  = j.value("primary", r.primary);
}

inline void to_json(nlohmann::json& j, const BridgeConfig& b)
{
    j = nlohmann::json{
        {"enabled",          b.enabled},
        {"source_device_id", b.sourceDeviceId},
        {"target_device_id", b.targetDeviceId},
    };
}

inline void from_json(const nlohmann::json& j, BridgeConfig& b)
{
    if (! j.is_object())
        return;
    b.enabled        = j.value("enabled", b.enabled);
    b.sourceDeviceId = j.value("source_device_id", b.sourceDeviceId);
    b.targetDeviceId = j.value("target_device_id", b.targetDeviceId);
}

inline void to_json(nlohmann::json& j, const HotkeysConfig& h)
{
    j = nlohmann::json::array();
    for (const auto& b : h.bindings)
        j.push_back({{"action", toString(b.action)},
                     {"key", b.hotkey.toString()},
                     {"enabled", b.enabled}});
}

inline void from_json(const nlohmann::json& j, HotkeysConfig& h)
{
    if (! j.is_array())
        return;
    h.bindings.clear();
    for (const auto& e : j)
    {
        if (! e.is_object())
            continue;
        HotkeyBinding b;
        b.action = hotkeyActionFromString(e.value("action", std::string{}));
        if (auto hk = Hotkey::parse(e.value("key", std::string{})))
            b.hotkey = *hk;
        b.enabled = e.value("enabled", true);
        if (b.action != HotkeyAction::None)
            h.bindings.push_back(b);
    }
}

inline void to_json(nlohmann::json& j, const SharingConfig& s)
{
    j = nlohmann::json{
        {"enabled", s.enabled},
        {"routes",  s.routes},
    };
}

inline void from_json(const nlohmann::json& j, SharingConfig& s)
{
    if (! j.is_object())
        return;
    s.enabled = j.value("enabled", s.enabled);
    s.routes.clear();
    if (const auto it = j.find("routes"); it != j.end() && it->is_array())
        for (const auto& e : *it)
        {
            OutputRoute r;
            from_json(e, r);
            if (! r.deviceId.empty())
                s.routes.push_back(std::move(r));
        }
}

} // namespace veyra
