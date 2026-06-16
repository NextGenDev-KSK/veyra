#pragma once

#include <filesystem>

#include "veyra/AppRules.h"
#include "veyra/ipc/PipeServer.h"

namespace veyra {
class Logger;
}

namespace veyra::service {

class ConfigManager;
class PresetLibrary;

// Serves the \\.\pipe\veyra-control channel: ping/pong, version, config
// get/set, preset list/load/save/delete, and per-app rule get/set.
class ControlServer {
public:
    ControlServer(ConfigManager& config, PresetLibrary& presets,
                  std::filesystem::path appRulesFile, Logger* log);

    bool start() { return server_.start(); }
    void stop()  { server_.stop(); }

    // Load persisted app rules into the engine (call once at startup).
    void loadAppRules();

    const AppRuleEngine& appRules() const noexcept { return rules_; }

private:
    ipc::Message handle(const ipc::Message& request);
    void         persistAppRules();

    ConfigManager&        config_;
    PresetLibrary&        presets_;
    AppRuleEngine         rules_;
    std::filesystem::path appRulesFile_;
    Logger*               log_;
    ipc::PipeServer       server_;
};

} // namespace veyra::service
