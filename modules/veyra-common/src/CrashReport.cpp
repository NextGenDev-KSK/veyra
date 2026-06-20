#include "veyra/CrashReport.h"

#include <fstream>
#include <sstream>
#include <system_error>

#include <nlohmann/json.hpp>

namespace veyra {

using nlohmann::json;

std::string CrashReport::toJson() const
{
    json j;
    j["version"]        = version;
    j["git_commit"]     = gitCommit;
    j["timestamp"]      = timestamp;
    j["process"]        = process;
    j["exception_code"] = exceptionCode;
    j["address"]        = address;
    j["module"]         = module;
    j["message"]        = message;
    return j.dump(2);
}

std::optional<CrashReport> CrashReport::fromJson(const std::string& text)
{
    json j = json::parse(text, nullptr, /*allow_exceptions=*/false);
    if (j.is_discarded() || !j.is_object())
        return std::nullopt;

    CrashReport r;
    r.version       = j.value("version", std::string{});
    r.gitCommit     = j.value("git_commit", std::string{});
    r.timestamp     = j.value("timestamp", std::string{});
    r.process       = j.value("process", std::string{});
    r.exceptionCode = j.value("exception_code", 0u);
    r.address       = j.value("address", (uint64_t) 0);
    r.module        = j.value("module", std::string{});
    r.message       = j.value("message", std::string{});
    return r;
}

namespace {
std::string fileStamp(const std::string& iso)
{
    // ISO timestamp -> filename-safe token.
    std::string s = iso;
    for (char& c : s)
        if (c == ':' || c == '.' || c == ' ')
            c = '-';
    return s.empty() ? "unknown" : s;
}
} // namespace

std::filesystem::path writeCrashReport(const CrashReport& report,
                                       const std::filesystem::path& crashesDir)
{
    std::error_code ec;
    std::filesystem::create_directories(crashesDir, ec);

    const auto file = crashesDir / ("crash-" + fileStamp(report.timestamp) + ".json");
    std::ofstream out(file, std::ios::binary | std::ios::trunc);
    if (!out)
        return {};
    out << report.toJson();
    if (!out)
        return {};
    return file;
}

std::optional<CrashReport> latestCrashReport(const std::filesystem::path& crashesDir)
{
    std::error_code ec;
    if (!std::filesystem::exists(crashesDir, ec))
        return std::nullopt;

    std::filesystem::path newest;
    std::filesystem::file_time_type newestTime{};
    for (std::filesystem::directory_iterator it(crashesDir, ec), end; it != end; it.increment(ec))
    {
        if (ec) break;
        const auto& p = it->path();
        if (p.extension() != ".json" || p.filename().string().rfind("crash-", 0) != 0)
            continue;
        const auto t = std::filesystem::last_write_time(p, ec);
        if (ec) continue;
        if (newest.empty() || t > newestTime) { newest = p; newestTime = t; }
    }
    if (newest.empty())
        return std::nullopt;

    std::ifstream in(newest, std::ios::binary);
    if (!in)
        return std::nullopt;
    std::ostringstream ss;
    ss << in.rdbuf();
    return CrashReport::fromJson(ss.str());
}

} // namespace veyra
