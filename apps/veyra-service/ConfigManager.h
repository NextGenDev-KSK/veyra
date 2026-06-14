#pragma once

#include <filesystem>
#include <mutex>
#include <string>

#include "veyra/Config.h"

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

private:
    std::filesystem::path file_;
    Logger*               log_;
    std::mutex            mutex_;
    Config                config_;
};

} // namespace veyra::service
