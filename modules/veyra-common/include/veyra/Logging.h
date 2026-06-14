#pragma once

// Thin logging facade. Implemented on spdlog (rotating file sink) in
// Logging.cpp, but spdlog is kept out of this header so callers don't pay its
// compile cost — the implementation is held behind an opaque pointer.

#include <filesystem>
#include <memory>
#include <string>
#include <string_view>

namespace veyra {

enum class LogLevel { Info, Warn, Error };

class Logger {
public:
    // Opens (or rotates into) the given log file, creating parent directories.
    // If the sink cannot be created, logging degrades to a silent no-op so it is
    // never fatal to the host process.
    explicit Logger(std::filesystem::path file);
    ~Logger();

    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    void log(LogLevel level, std::string_view message);

    void info(std::string_view message)  { log(LogLevel::Info, message); }
    void warn(std::string_view message)  { log(LogLevel::Warn, message); }
    void error(std::string_view message) { log(LogLevel::Error, message); }

private:
    std::shared_ptr<void> impl_; // spdlog::logger, opaque to keep spdlog private
};

} // namespace veyra
