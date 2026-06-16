#pragma once

// Canonical application configuration (subset of engineering-spec §11 needed for
// Phase 1; fields are added as later phases bring features online). JSON is kept
// out of this header so consumers don't take a dependency on nlohmann.

#include <array>
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

// Live audio enhancement parameters surfaced by the Home screen. Ranges are
// chosen to map cleanly onto veyra::dsp::DspParameters; the service's
// ApoPublisher does the final translation into the shared-memory payload.
struct EnhancementConfig {
    std::array<float, 10> eqBandsDb{}; // graphic EQ, -12..+12 dB per band
    std::string           eqMode = "graphic"; // graphic | parametric
    float bassBoostDb       = 0.0f; // low shelf, 0..+12 dB
    float trebleDb          = 0.0f; // high shelf, 0..+12 dB
    float volumeGain        = 1.0f; // pre-amp knob, 0..3 (×master trim)
    float stereoWidth       = 1.0f; // 0 (mono) .. 2 (wide)
    float compressionAmount = 0.0f; // 0..1
    float reverbAmount      = 0.0f; // 0..1 — no DSP stage yet, persisted for the UI
};

// Spatial / headphone parameters. crossfeed is the realtime render-side effect;
// mode is remembered for the UI (0 off, 1 cinematic, 2 competitive). Full HRTF
// virtual surround arrives with a multichannel virtual endpoint in a later pass.
struct SpatialConfig {
    bool  enabled   = false;
    float crossfeed = 0.0f; // 0..1
    int   mode      = 0;    // 0 off, 1 cinematic, 2 competitive
};

// Microphone (capture) chain parameters; mirrors veyra::dsp::VoiceParams. The
// service maps this onto the mic shared-memory block read by the capture APO.
struct VoiceConfig {
    bool  enabled          = true;
    float highPassHz       = 80.0f;
    float noiseSuppression = 0.5f; // 0..1
    float compressionAmount = 0.3f; // 0..1
    float deEssAmount      = 0.3f;  // 0..1
    float presenceDb       = 2.0f;
    float outputGainDb     = 0.0f;
    float sideToneLevel    = 0.0f;  // 0..1
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
    EnhancementConfig enhancement;
    VoiceConfig       voice;
    SpatialConfig     spatial;

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
