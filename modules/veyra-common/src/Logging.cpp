#include "veyra/Logging.h"

#include <chrono>
#include <ctime>
#include <system_error>

namespace veyra {

namespace {

const char* levelTag(LogLevel level)
{
    switch (level)
    {
    case LogLevel::Info:  return "INFO";
    case LogLevel::Warn:  return "WARN";
    case LogLevel::Error: return "ERROR";
    }
    return "INFO";
}

std::string timestamp()
{
    using namespace std::chrono;
    const auto now = system_clock::now();
    const auto secs = system_clock::to_time_t(now);
    const auto ms = duration_cast<milliseconds>(now.time_since_epoch()) % 1000;

    std::tm tm{};
    localtime_s(&tm, &secs);

    char buf[32];
    std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &tm);

    char out[40];
    std::snprintf(out, sizeof(out), "%s.%03lld", buf, static_cast<long long>(ms.count()));
    return out;
}

} // namespace

Logger::Logger(std::filesystem::path file)
{
    std::error_code ec;
    std::filesystem::create_directories(file.parent_path(), ec); // best-effort
    out_.open(file, std::ios::out | std::ios::app);
}

void Logger::log(LogLevel level, std::string_view message)
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (!out_.is_open())
        return;
    out_ << timestamp() << " [" << levelTag(level) << "] " << message << '\n';
    out_.flush();
}

} // namespace veyra
