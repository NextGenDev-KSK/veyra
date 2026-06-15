#include "veyra/Preset.h"

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

std::string Preset::toJson() const
{
    json j;
    j["schema"]             = schema;
    j["uuid"]               = uuid;
    j["name"]               = name;
    j["category"]           = category;
    j["author"]             = author;
    j["builtin"]            = builtIn;
    j["master_volume_gain"] = masterVolumeGain;
    j["enhancement"]        = enhancement;
    return j.dump(2);
}

std::optional<Preset> Preset::fromJson(const std::string& text)
{
    json j = json::parse(text, nullptr, /*allow_exceptions=*/false);
    if (j.is_discarded() || !j.is_object())
        return std::nullopt;

    Preset p;
    p.schema           = j.value("schema", p.schema);
    p.uuid             = j.value("uuid", p.uuid);
    p.name             = j.value("name", p.name);
    p.category         = j.value("category", p.category);
    p.author           = j.value("author", p.author);
    p.builtIn          = j.value("builtin", p.builtIn);
    p.masterVolumeGain = j.value("master_volume_gain", p.masterVolumeGain);
    if (const auto it = j.find("enhancement"); it != j.end() && it->is_object())
        from_json(*it, p.enhancement);

    if (p.uuid.empty() || p.name.empty())
        return std::nullopt; // a preset must be identifiable
    return p;
}

std::optional<Preset> Preset::load(const std::filesystem::path& file)
{
    std::ifstream in(file, std::ios::binary);
    if (!in)
        return std::nullopt;
    std::ostringstream ss;
    ss << in.rdbuf();
    return fromJson(ss.str());
}

bool Preset::save(const std::filesystem::path& file) const
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
    if (!MoveFileExW(tmp.c_str(), file.c_str(),
                     MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH))
    {
        std::filesystem::remove(tmp, ec);
        return false;
    }
    return true;
}

void Preset::applyTo(Config& c) const
{
    c.masterVolumeGain = masterVolumeGain;
    c.enhancement      = enhancement;
    c.activePresetUuid = uuid;
}

// ---------------------------------------------------------------------------
// Built-in presets. Stable UUIDs (v-prefixed) so saved active-preset ids and
// per-app rules keep resolving across versions.
// EQ band order: 32 64 125 250 500 1k 2k 4k 8k 16k (Hz).
// ---------------------------------------------------------------------------
namespace {

Preset makeBuiltIn(const char* uuid, const char* name, const char* category,
                   std::array<float, 10> eq, float bass, float treble,
                   float comp, float width, float reverb)
{
    Preset p;
    p.uuid             = uuid;
    p.name             = name;
    p.category         = category;
    p.author           = "Veyra";
    p.builtIn          = true;
    p.masterVolumeGain = 1.0;
    p.enhancement.eqBandsDb        = eq;
    p.enhancement.bassBoostDb      = bass;
    p.enhancement.trebleDb         = treble;
    p.enhancement.compressionAmount = comp;
    p.enhancement.stereoWidth      = width;
    p.enhancement.reverbAmount     = reverb;
    return p;
}

} // namespace

const std::vector<Preset>& builtInPresets()
{
    static const std::vector<Preset> kPresets = {
        makeBuiltIn("v-flat", "Flat", "General",
                    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f),
        makeBuiltIn("v-bass-boost", "Bass Boost", "Music",
                    {6, 5, 3, 1, 0, 0, 0, 0, 0, 0}, 5.0f, 0.0f, 0.10f, 1.0f, 0.0f),
        makeBuiltIn("v-bass-reducer", "Bass Reducer", "Music",
                    {-6, -5, -3, -1, 0, 0, 0, 0, 0, 0}, -4.0f, 0.0f, 0.0f, 1.0f, 0.0f),
        makeBuiltIn("v-treble-boost", "Treble Boost", "Music",
                    {0, 0, 0, 0, 0, 1, 3, 5, 6, 6}, 0.0f, 5.0f, 0.0f, 1.0f, 0.0f),
        makeBuiltIn("v-vocal-clarity", "Vocal Clarity", "Voice",
                    {-2, -1, 0, 2, 4, 4, 3, 2, 1, 0}, 0.0f, 2.0f, 0.25f, 0.9f, 0.0f),
        makeBuiltIn("v-fps-footsteps", "FPS Footstep Boost", "Gaming",
                    {-3, -2, 0, 2, 5, 6, 4, 2, 0, -2}, 0.0f, 2.0f, 0.45f, 1.1f, 0.0f),
        makeBuiltIn("v-cinematic", "Cinematic", "Movies",
                    {4, 3, 1, 0, 0, 1, 2, 3, 3, 2}, 3.0f, 2.0f, 0.20f, 1.4f, 0.15f),
        makeBuiltIn("v-night", "Night", "General",
                    {2, 1, 0, 0, 1, 1, 0, -1, -2, -3}, 1.0f, -2.0f, 0.60f, 0.9f, 0.0f),
    };
    return kPresets;
}

} // namespace veyra
