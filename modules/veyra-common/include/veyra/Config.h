#pragma once

// Canonical application configuration (subset of engineering-spec §11 needed for
// Phase 1; fields are added as later phases bring features online). JSON is kept
// out of this header so consumers don't take a dependency on nlohmann.

#include <filesystem>
#include <optional>
#include <string>

namespace veyra {

struct AudioEngineConfig {
    int         sampleRate  = 48000;
    int         bitDepth    = 24;
    int         bufferSize  = 128;
    std::string latencyMode = "Standard"; // Standard | UltraLow
    std::string dspPrecision = "float32";
};

struct Config {
    int         version            = 1;
    bool        masterEnabled      = true;
    double      masterVolumeGain   = 1.0;
    std::string activePresetUuid;
    std::string theme              = "midnight";
    std::string language           = "en";
    bool        telemetryOptIn     = false;
    AudioEngineConfig audioEngine;

    // Serialise to / from pretty-printed JSON text.
    std::string toJson() const;
    static std::optional<Config> fromJson(const std::string& text);

    // Load from disk; std::nullopt if the file is missing or unparsable.
    static std::optional<Config> load(const std::filesystem::path& file);

    // Atomic save: write a temp file then replace the target, so a crash mid
    // write can never corrupt the live config. Returns false on I/O failure.
    bool save(const std::filesystem::path& file) const;
};

} // namespace veyra
