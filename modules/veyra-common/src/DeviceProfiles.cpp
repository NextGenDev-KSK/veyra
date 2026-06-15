#include "veyra/DeviceProfiles.h"

#include <fstream>
#include <sstream>
#include <system_error>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

#include <nlohmann/json.hpp>

namespace veyra {

using nlohmann::json;

void DeviceProfiles::remember(const std::string& deviceId, const Config& c)
{
    if (!deviceId.empty())
        profiles_[deviceId] = c;
}

std::optional<Config> DeviceProfiles::recall(const std::string& deviceId) const
{
    if (const auto it = profiles_.find(deviceId); it != profiles_.end())
        return it->second;
    return std::nullopt;
}

bool DeviceProfiles::has(const std::string& deviceId) const
{
    return profiles_.find(deviceId) != profiles_.end();
}

std::string DeviceProfiles::toJson() const
{
    json devices = json::object();
    for (const auto& [id, cfg] : profiles_)
        devices[id] = json::parse(cfg.toJson(), nullptr, /*allow_exceptions=*/false);
    json j;
    j["schema"]  = 1;
    j["devices"] = std::move(devices);
    return j.dump(2);
}

DeviceProfiles DeviceProfiles::fromJson(const std::string& text)
{
    DeviceProfiles out;
    json j = json::parse(text, nullptr, /*allow_exceptions=*/false);
    if (j.is_discarded() || !j.is_object())
        return out;

    const auto it = j.find("devices");
    if (it == j.end() || !it->is_object())
        return out;

    for (const auto& [id, cfg] : it->items())
        if (auto parsed = Config::fromJson(cfg.dump()))
            out.profiles_[id] = *parsed;
    return out;
}

bool DeviceProfiles::save(const std::filesystem::path& file) const
{
    std::error_code ec;
    std::filesystem::create_directories(file.parent_path(), ec);

    const std::filesystem::path tmp = file.wstring() + L".tmp";
    {
        std::ofstream out(tmp, std::ios::binary | std::ios::trunc);
        if (!out)
            return false;
        out << toJson();
        if (!out)
            return false;
    }
    if (!MoveFileExW(tmp.c_str(), file.c_str(),
                     MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH))
    {
        std::filesystem::remove(tmp, ec);
        return false;
    }
    return true;
}

DeviceProfiles DeviceProfiles::load(const std::filesystem::path& file)
{
    std::ifstream in(file, std::ios::binary);
    if (!in)
        return {};
    std::ostringstream ss;
    ss << in.rdbuf();
    return fromJson(ss.str());
}

} // namespace veyra
