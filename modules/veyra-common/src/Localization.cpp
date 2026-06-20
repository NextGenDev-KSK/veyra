#include "veyra/Localization.h"

#include <fstream>
#include <sstream>

#include <nlohmann/json.hpp>

namespace veyra {

using nlohmann::json;

Localization::Localization()
{
    // English base catalog (authoritative). Other languages override these keys
    // via resources/lang/<code>.json; untranslated keys fall back to English.
    base_ = {
        {"app.title", "Veyra Sounds"},
        {"nav.home", "Home"},
        {"nav.presets", "Presets"},
        {"nav.apps", "Apps"},
        {"nav.devices", "Devices"},
        {"nav.soundLab", "Sound Lab"},
        {"nav.gamerMode", "Gamer Mode"},
        {"nav.settings", "Settings"},
        {"common.enable", "Enable"},
        {"common.reset", "Reset"},
        {"common.save", "Save"},
        {"common.cancel", "Cancel"},
        {"common.apply", "Apply"},
        {"common.on", "On"},
        {"common.off", "Off"},
        {"master.label", "Master"},
        {"home.equalizer", "Equalizer"},
        {"home.moreEffects", "More Effects"},
        {"settings.appearance", "Appearance"},
        {"settings.microphone", "Microphone"},
        {"settings.spatial", "Spatial"},
        {"settings.loudness", "Loudness"},
        {"settings.about", "About"},
        {"loudness.nightMode", "Night Mode"},
        {"loudness.sleepTimer", "Sleep Timer"},
        {"loudness.match", "Loudness Match"},
        {"spatial.crossfeed", "Crossfeed"},
        {"spatial.virtualization", "Virtualization"},
        {"devices.audioBridge", "Audio Bridge"},
        {"miniMode", "Mini Mode"},
    };
}

void Localization::setOverlayJson(const std::string& jsonText)
{
    overlay_.clear();
    json j = json::parse(jsonText, nullptr, /*allow_exceptions=*/false);
    if (j.is_discarded() || !j.is_object())
        return;
    for (const auto& [k, v] : j.items())
        if (v.is_string())
            overlay_[k] = v.get<std::string>();
}

bool Localization::setLanguage(const std::string& code, const std::filesystem::path& langDir)
{
    lang_ = code;
    if (code.empty() || code == "en")
    {
        clearOverlay();
        return true;
    }

    std::ifstream in(langDir / (code + ".json"), std::ios::binary);
    if (!in)
    {
        clearOverlay();
        return false;
    }
    std::ostringstream ss;
    ss << in.rdbuf();
    setOverlayJson(ss.str());
    return true;
}

void Localization::clearOverlay() { overlay_.clear(); }

std::string Localization::tr(const std::string& key) const
{
    if (const auto it = overlay_.find(key); it != overlay_.end())
        return it->second;
    if (const auto it = base_.find(key); it != base_.end())
        return it->second;
    return key; // surface the missing key rather than an empty string
}

Localization& loc()
{
    static Localization instance;
    return instance;
}

} // namespace veyra
