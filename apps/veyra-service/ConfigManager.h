#pragma once

#include <filesystem>
#include <functional>
#include <mutex>
#include <string>

#include "veyra/Config.h"
#include "veyra/Preset.h"

namespace veyra {
class Logger;
}

namespace veyra::service {

// Owns the canonical Config and serialises access to it. Thread-safe: the
// control server calls into it from the pipe listener thread.
class ConfigManager {
public:
    ConfigManager(std::filesystem::path file, Logger* log);

    // Load from disk, or write+verify a default config if none exists.
    void loadOrCreateDefault();

    std::string getJson();                  // snapshot as JSON
    bool        setJson(const std::string&); // parse, store, persist
    Config      current();                   // snapshot as a Config

    // Apply a preset onto the current config, then persist + notify.
    bool        applyPreset(const Preset& preset);

    // Invoked (with the current Config) after the config is loaded or changed.
    // Used by the service to republish DSP parameters to the APO.
    void setOnChanged(std::function<void(const Config&)> cb) { onChanged_ = std::move(cb); }

private:
    void notifyChanged(); // call onChanged_ with a snapshot (no lock held)

    std::filesystem::path             file_;
    Logger*                           log_;
    std::mutex                        mutex_;
    Config                            config_;
    std::function<void(const Config&)> onChanged_;
};

} // namespace veyra::service
