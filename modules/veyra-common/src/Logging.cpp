#include "veyra/Logging.h"

#include <vector>

#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

namespace veyra {

namespace {
spdlog::level::level_enum toSpd(LogLevel level)
{
    switch (level)
    {
    case LogLevel::Info:  return spdlog::level::info;
    case LogLevel::Warn:  return spdlog::level::warn;
    case LogLevel::Error: return spdlog::level::err;
    }
    return spdlog::level::info;
}
} // namespace

Logger::Logger(std::filesystem::path file, bool alsoConsole)
{
    try
    {
        std::error_code ec;
        std::filesystem::create_directories(file.parent_path(), ec); // best-effort

        std::vector<spdlog::sink_ptr> sinks;
        // 5 MB x 3 rotated files keeps disk use bounded without external config.
        sinks.push_back(std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
            file.string(), 5 * 1024 * 1024, 3));
        if (alsoConsole)
            sinks.push_back(std::make_shared<spdlog::sinks::stdout_color_sink_mt>());

        auto logger = std::make_shared<spdlog::logger>("veyra", sinks.begin(), sinks.end());
        logger->set_level(spdlog::level::trace);
        logger->flush_on(spdlog::level::info);
        logger->set_pattern("%Y-%m-%d %H:%M:%S.%e [%^%l%$] %v");
        impl_ = std::move(logger);
    }
    catch (...)
    {
        impl_ = nullptr; // silent no-op on failure
    }
}

Logger::~Logger() = default;

void Logger::log(LogLevel level, std::string_view message)
{
    if (!impl_)
        return;
    auto* logger = static_cast<spdlog::logger*>(impl_.get());
    // "{}" so message text is data, never interpreted as a format string.
    logger->log(toSpd(level), "{}", message);
}

} // namespace veyra
