#pragma once

// Canonical application configuration (subset of engineering-spec §11 needed for
// Phase 1; fields are added as later phases bring features online). JSON is kept
// out of this header so consumers don't take a dependency on nlohmann.

#include <array>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

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
    bool  enabled        = false;
    float crossfeed      = 0.0f; // 0..1 headphone crossfeed
    float virtualization = 0.0f; // 0..1 HRTF out-of-head virtualisation
    int   mode           = 0;    // 0 off, 1 cinematic, 2 competitive
};

// Gamer Mode (Sound Tracker) settings. The service runs the tracker over a
// loopback capture when enabled; the overlay reads these to choose its radar.
struct GamerModeConfig {
    bool  enabled     = false;
    float sensitivity = 0.6f; // 0..1
    int   radarMode   = 0;    // 0 competitive, 1 rich, 2 compass
    bool  footsteps   = true;
    bool  gunshots    = true;
    bool  voices      = false;
};

// Loudness / late-night settings. nightModeAmount drives the render-side Night
// Mode compressor; the sleep timer is driven by the service, which fades the
// output to silence over sleepFadeSeconds once sleepTimerMinutes elapses.
struct LoudnessConfig {
    float nightModeAmount   = 0.0f;  // 0 = off .. 1
    bool  sleepTimerEnabled = false;
    float sleepTimerMinutes = 30.0f; // time until the fade begins
    float sleepFadeSeconds  = 20.0f; // fade-out tail length
};

// Audio bridge (no-driver processing path): when enabled, the service loopback-
// captures 'sourceDeviceId' (set this as the Windows default so apps play into
// it — typically a virtual sink), runs the DSP, and renders to 'targetDeviceId'
// (your real headphones). Empty ids = use the system default endpoint.
struct BridgeConfig {
    bool        enabled = false;
    std::string sourceDeviceId; // captured (loopback)
    std::string targetDeviceId; // processed output is rendered here
};

// One destination in Sound Sharing (multi-output). The service opens each
// enabled route's endpoint, applies the per-output trim, and delays it by
// delayMs so all routes stay in sync with the primary (the reference endpoint).
struct OutputRoute {
    std::string deviceId;          // endpoint id
    std::string name;              // friendly name (for the UI)
    bool        enabled = true;
    float       gainDb  = 0.0f;    // per-output trim
    float       delayMs = 0.0f;    // latency compensation
    bool        primary = false;   // the reference others align to
};

struct SharingConfig {
    bool                     enabled = false;
    std::vector<OutputRoute> routes;
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
    double      uiOpacity          = 0.85;  // glass translucency 0.3..1
    int         backgroundMode     = 0;     // 0 ambient, 1 solid, 2 image
    bool        reduceMotion       = false; // freeze visualizer/transitions
    std::string language           = "en";
    bool        telemetryOptIn     = false;
    AudioEngineConfig audioEngine;
    EnhancementConfig enhancement;
    VoiceConfig       voice;
    SpatialConfig     spatial;
    GamerModeConfig   gamerMode;
    LoudnessConfig    loudness;
    SharingConfig     sharing;
    BridgeConfig      bridge;

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
