#pragma once

// A `.vpreset` is a named, portable bundle of enhancement settings (the same
// EnhancementConfig the live engine uses, plus a master gain and metadata).
// Built-in presets ship with the app; user presets are saved as individual
// JSON files under %APPDATA%\Veyra\presets. JSON stays out of this header so
// consumers don't take a dependency on nlohmann.

#include <filesystem>
#include <optional>
#include <string>
#include <vector>

#include "veyra/Config.h"

namespace veyra {

struct Preset {
    int               schema = 1;
    std::string       uuid;
    std::string       name;
    std::string       category = "Custom";
    std::string       author   = "User";
    bool              builtIn  = false;
    double            masterVolumeGain = 1.0;
    EnhancementConfig enhancement;

    std::string toJson() const;
    static std::optional<Preset> fromJson(const std::string& text);

    static std::optional<Preset> load(const std::filesystem::path& file);
    bool save(const std::filesystem::path& file) const; // atomic write

    // Apply the preset's gains/enhancement onto a Config (sets activePresetUuid).
    void applyTo(Config& c) const;
};

// The curated set shipped with the app (stable UUIDs, in display order).
const std::vector<Preset>& builtInPresets();

} // namespace veyra
