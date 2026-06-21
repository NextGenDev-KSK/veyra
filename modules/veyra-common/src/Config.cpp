#include "veyra/Config.h"

#include <fstream>
#include <sstream>
#include <system_error>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

#include <nlohmann/json.hpp>

#include "ConfigJson.h"

namespace veyra {

using nlohmann::json;

std::string Config::toJson() const
{
    json j;
    j["version"]            = version;
    j["master_enabled"]     = masterEnabled;
    j["master_volume_gain"] = masterVolumeGain;
    j["active_preset_uuid"] = activePresetUuid;
    j["appearance"]         = {
        {"theme",           theme},
        {"ui_opacity",      uiOpacity},
        {"background_mode", backgroundMode},
        {"reduce_motion",   reduceMotion},
    };
    j["language"]           = language;
    j["telemetry_opt_in"]   = telemetryOptIn;
    j["onboarding_complete"] = onboardingComplete;
    j["app_switching"]      = appSwitching;
    j["audio_engine"]       = {
        {"sample_rate",   audioEngine.sampleRate},
        {"bit_depth",     audioEngine.bitDepth},
        {"buffer_size",   audioEngine.bufferSize},
        {"latency_mode",  audioEngine.latencyMode},
        {"dsp_precision", audioEngine.dspPrecision},
        {"hardware_acceleration", audioEngine.hardwareAcceleration},
    };
    j["enhancement"]        = enhancement;
    j["voice"]              = voice;
    j["spatial"]            = spatial;
    j["gamer_mode"]         = gamerMode;
    j["loudness"]           = loudness;
    j["sharing"]            = sharing;
    j["bridge"]             = bridge;
    j["hotkeys"]            = hotkeys;
    return j.dump(2);
}

std::optional<Config> Config::fromJson(const std::string& text)
{
    json j = json::parse(text, nullptr, /*allow_exceptions=*/false);
    if (j.is_discarded() || !j.is_object())
        return std::nullopt;

    Config c;
    c.version          = j.value("version", c.version);
    c.masterEnabled    = j.value("master_enabled", c.masterEnabled);
    c.masterVolumeGain = j.value("master_volume_gain", c.masterVolumeGain);
    c.activePresetUuid = j.value("active_preset_uuid", c.activePresetUuid);
    c.language         = j.value("language", c.language);
    c.telemetryOptIn   = j.value("telemetry_opt_in", c.telemetryOptIn);
    c.onboardingComplete = j.value("onboarding_complete", c.onboardingComplete);
    c.appSwitching     = j.value("app_switching", c.appSwitching);

    if (const auto it = j.find("appearance"); it != j.end() && it->is_object())
    {
        c.theme          = it->value("theme", c.theme);
        c.uiOpacity      = it->value("ui_opacity", c.uiOpacity);
        c.backgroundMode = it->value("background_mode", c.backgroundMode);
        c.reduceMotion   = it->value("reduce_motion", c.reduceMotion);
    }

    if (const auto it = j.find("audio_engine"); it != j.end() && it->is_object())
    {
        const auto& a = *it;
        c.audioEngine.sampleRate   = a.value("sample_rate", c.audioEngine.sampleRate);
        c.audioEngine.bitDepth     = a.value("bit_depth", c.audioEngine.bitDepth);
        c.audioEngine.bufferSize   = a.value("buffer_size", c.audioEngine.bufferSize);
        c.audioEngine.latencyMode  = a.value("latency_mode", c.audioEngine.latencyMode);
        c.audioEngine.dspPrecision = a.value("dsp_precision", c.audioEngine.dspPrecision);
        c.audioEngine.hardwareAcceleration = a.value("hardware_acceleration", c.audioEngine.hardwareAcceleration);
    }

    if (const auto it = j.find("enhancement"); it != j.end() && it->is_object())
        from_json(*it, c.enhancement);
    if (const auto it = j.find("voice"); it != j.end() && it->is_object())
        from_json(*it, c.voice);
    if (const auto it = j.find("spatial"); it != j.end() && it->is_object())
        from_json(*it, c.spatial);
    if (const auto it = j.find("gamer_mode"); it != j.end() && it->is_object())
        from_json(*it, c.gamerMode);
    if (const auto it = j.find("loudness"); it != j.end() && it->is_object())
        from_json(*it, c.loudness);
    if (const auto it = j.find("sharing"); it != j.end() && it->is_object())
        from_json(*it, c.sharing);
    if (const auto it = j.find("bridge"); it != j.end() && it->is_object())
        from_json(*it, c.bridge);
    if (const auto it = j.find("hotkeys"); it != j.end() && it->is_array())
        from_json(*it, c.hotkeys);
    return c;
}

std::optional<Config> Config::load(const std::filesystem::path& file)
{
    std::ifstream in(file, std::ios::binary);
    if (!in)
        return std::nullopt;

    std::ostringstream ss;
    ss << in.rdbuf();
    return fromJson(ss.str());
}

bool Config::save(const std::filesystem::path& file) const
{
    std::error_code ec;
    std::filesystem::create_directories(file.parent_path(), ec);

    const std::filesystem::path tmp = file.wstring() + L".tmp";
    {
        std::ofstream out(tmp, std::ios::binary | std::ios::trunc);
        if (!out)
            return false;
        out << toJson();
        if (!out)
            return false;
    }

    // Atomic replace; MoveFileEx is the Win32 way to overwrite the target.
    if (!MoveFileExW(tmp.c_str(), file.c_str(),
                     MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH))
    {
        std::filesystem::remove(tmp, ec);
        return false;
    }
    return true;
}

} // namespace veyra
