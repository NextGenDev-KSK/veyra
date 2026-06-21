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

std::string presetsToJsonArray(const std::vector<Preset>& presets)
{
    json arr = json::array();
    for (const auto& p : presets)
        arr.push_back(json::parse(p.toJson(), nullptr, /*allow_exceptions=*/false));
    return arr.dump(2);
}

std::vector<Preset> presetsFromJsonArray(const std::string& text)
{
    std::vector<Preset> out;
    json j = json::parse(text, nullptr, /*allow_exceptions=*/false);
    if (j.is_discarded() || !j.is_array())
        return out;
    for (const auto& e : j)
        if (e.is_object())
            if (auto p = Preset::fromJson(e.dump()))
                out.push_back(std::move(*p));
    return out;
}

const std::vector<Preset>& builtInPresets()
{
    // EQ bands: 32 64 125 250 500 1k 2k 4k 8k 16k (Hz).
    static const std::vector<Preset> kPresets = {
        // --- Gaming ---
        makeBuiltIn("v-fps-competitive", "FPS Competitive", "Gaming",
                    {-3, -2, 0, 2, 5, 6, 4, 2, 0, -2}, 0.0f, 2.0f, 0.45f, 1.10f, 0.0f),
        makeBuiltIn("v-footstep-boost", "Footstep Boost", "Gaming",
                    {-4, -3, -1, 2, 6, 7, 5, 2, 0, -3}, 0.0f, 2.0f, 0.50f, 1.10f, 0.0f),
        makeBuiltIn("v-valorant-precision", "Valorant Precision", "Gaming",
                    {-3, -2, 0, 3, 5, 5, 4, 3, 1, -1}, 0.0f, 3.0f, 0.40f, 1.00f, 0.0f),
        makeBuiltIn("v-cs2-tactical", "CS2 Tactical", "Gaming",
                    {-3, -2, 0, 2, 4, 6, 5, 3, 1, -2}, 0.0f, 2.0f, 0.40f, 1.05f, 0.0f),
        makeBuiltIn("v-warzone-focus", "Warzone Focus", "Gaming",
                    {-2, -1, 0, 2, 4, 5, 4, 3, 2, 0}, 0.0f, 2.0f, 0.35f, 1.10f, 0.0f),
        makeBuiltIn("v-apex-awareness", "Apex Awareness", "Gaming",
                    {-3, -1, 0, 2, 5, 5, 4, 3, 2, -1}, 0.0f, 2.0f, 0.40f, 1.15f, 0.0f),
        makeBuiltIn("v-pubg-competitive", "PUBG Competitive", "Gaming",
                    {-3, -2, 0, 2, 5, 6, 4, 2, 1, -2}, 0.0f, 2.0f, 0.45f, 1.05f, 0.0f),

        // --- Music ---
        makeBuiltIn("v-studio-flat", "Studio Flat", "Music",
                    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f),
        makeBuiltIn("v-audiophile-reference", "Audiophile Reference", "Music",
                    {1, 1, 0, 0, 0, 0, 0, 0, 1, 1}, 1.0f, 1.0f, 0.10f, 1.0f, 0.0f),
        makeBuiltIn("v-edm-energy", "EDM Energy", "Music",
                    {6, 5, 2, 0, -1, 0, 1, 2, 4, 5}, 5.0f, 4.0f, 0.20f, 1.30f, 0.05f),
        makeBuiltIn("v-hiphop-bass", "Hip-Hop Bass", "Music",
                    {7, 6, 3, 1, 0, 0, 0, 1, 2, 2}, 6.0f, 1.0f, 0.25f, 1.10f, 0.0f),
        makeBuiltIn("v-rock-arena", "Rock Arena", "Music",
                    {4, 3, 1, 0, 1, 2, 2, 3, 3, 2}, 3.0f, 3.0f, 0.20f, 1.30f, 0.10f),
        makeBuiltIn("v-pop-clarity", "Pop Clarity", "Music",
                    {2, 1, 0, 1, 2, 3, 3, 3, 2, 1}, 1.0f, 3.0f, 0.15f, 1.10f, 0.0f),
        makeBuiltIn("v-bass-monster", "Bass Monster", "Music",
                    {9, 8, 5, 2, 0, 0, 0, 0, 0, 0}, 9.0f, 0.0f, 0.20f, 1.0f, 0.0f),
        makeBuiltIn("v-loudness-max", "Loudness Max", "Music",
                    {3, 2, 1, 1, 2, 2, 2, 3, 3, 3}, 3.0f, 3.0f, 0.50f, 1.10f, 0.0f),
        makeBuiltIn("v-clarity-focus", "Clarity Focus", "Music",
                    {-1, -1, 0, 1, 3, 4, 4, 3, 2, 1}, 0.0f, 3.0f, 0.20f, 1.0f, 0.0f),

        // --- Movies ---
        makeBuiltIn("v-cinema", "Cinema", "Movies",
                    {4, 3, 1, 0, 0, 1, 2, 3, 3, 2}, 3.0f, 2.0f, 0.20f, 1.40f, 0.15f),
        makeBuiltIn("v-dialogue-boost", "Dialogue Boost", "Movies",
                    {-2, -1, 0, 2, 4, 5, 4, 2, 1, 0}, 0.0f, 2.0f, 0.30f, 0.90f, 0.0f),
        makeBuiltIn("v-action-theater", "Action Theater", "Movies",
                    {6, 5, 2, 0, 0, 1, 2, 3, 4, 3}, 5.0f, 2.0f, 0.25f, 1.50f, 0.20f),
        makeBuiltIn("v-movie-surround", "Movie Surround", "Movies",
                    {3, 2, 1, 0, 1, 2, 2, 3, 3, 2}, 2.0f, 2.0f, 0.20f, 1.60f, 0.15f),

        // --- Streaming ---
        makeBuiltIn("v-streaming-voice", "Streaming Voice", "Streaming",
                    {-2, -1, 0, 2, 3, 4, 4, 3, 2, 1}, 0.0f, 2.0f, 0.30f, 1.0f, 0.0f),

        // --- Voice ---
        makeBuiltIn("v-vocal-boost", "Vocal Boost", "Voice",
                    {-2, -1, 0, 2, 4, 4, 3, 2, 1, 0}, 0.0f, 2.0f, 0.25f, 0.90f, 0.0f),
        makeBuiltIn("v-podcast-voice", "Podcast Voice", "Voice",
                    {-2, -1, 0, 2, 3, 4, 3, 2, 1, 0}, 0.0f, 2.0f, 0.30f, 0.90f, 0.0f),
        makeBuiltIn("v-conference-voice", "Conference Voice", "Voice",
                    {-3, -2, 0, 3, 4, 4, 3, 1, 0, -1}, 0.0f, 1.0f, 0.35f, 0.80f, 0.0f),

        // --- Night ---
        makeBuiltIn("v-night-listening", "Night Listening", "Night",
                    {2, 1, 0, 0, 1, 1, 0, -1, -2, -3}, 1.0f, -2.0f, 0.60f, 0.90f, 0.0f),
        makeBuiltIn("v-night-gaming", "Night Gaming", "Night",
                    {-2, -1, 0, 2, 4, 4, 3, 1, -1, -3}, 0.0f, 1.0f, 0.60f, 1.0f, 0.0f),
    };
    return kPresets;
}

} // namespace veyra
