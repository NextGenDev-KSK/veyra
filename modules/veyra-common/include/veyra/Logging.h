#pragma once

// Minimal thread-safe file logger for Phase 1. Replaced by spdlog in Phase 3
// (see CONTRIBUTING/BUILD_GUIDE); the call sites here use a tiny surface so the
// swap is mechanical.

#include <filesystem>
#include <fstream>
#include <mutex>
#include <string>
#include <string_view>

namespace veyra {

enum class LogLevel { Info, Warn, Error };

class Logger {
public:
    // Opens (appending) the given log file, creating parent directories. If the
    // file cannot be opened, logging silently degrades to a no-op so logging is
    // never fatal to the host process.
    explicit Logger(std::filesystem::path file);

    void log(LogLevel level, std::string_view message);

    void info(std::string_view message)  { log(LogLevel::Info, message); }
    void warn(std::string_view message)  { log(LogLevel::Warn, message); }
    void error(std::string_view message) { log(LogLevel::Error, message); }

private:
    std::mutex    mutex_;
    std::ofstream out_;
};

} // namespace veyra
