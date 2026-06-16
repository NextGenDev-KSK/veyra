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
    j["appearance"]         = { {"theme", theme} };
    j["language"]           = language;
    j["telemetry_opt_in"]   = telemetryOptIn;
    j["audio_engine"]       = {
        {"sample_rate",   audioEngine.sampleRate},
        {"bit_depth",     audioEngine.bitDepth},
        {"buffer_size",   audioEngine.bufferSize},
        {"latency_mode",  audioEngine.latencyMode},
        {"dsp_precision", audioEngine.dspPrecision},
    };
    j["enhancement"]        = enhancement;
    j["voice"]              = voice;
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

    if (const auto it = j.find("appearance"); it != j.end() && it->is_object())
        c.theme = it->value("theme", c.theme);

    if (const auto it = j.find("audio_engine"); it != j.end() && it->is_object())
    {
        const auto& a = *it;
        c.audioEngine.sampleRate   = a.value("sample_rate", c.audioEngine.sampleRate);
        c.audioEngine.bitDepth     = a.value("bit_depth", c.audioEngine.bitDepth);
        c.audioEngine.bufferSize   = a.value("buffer_size", c.audioEngine.bufferSize);
        c.audioEngine.latencyMode  = a.value("latency_mode", c.audioEngine.latencyMode);
        c.audioEngine.dspPrecision = a.value("dsp_precision", c.audioEngine.dspPrecision);
    }

    if (const auto it = j.find("enhancement"); it != j.end() && it->is_object())
        from_json(*it, c.enhancement);
    if (const auto it = j.find("voice"); it != j.end() && it->is_object())
        from_json(*it, c.voice);
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
