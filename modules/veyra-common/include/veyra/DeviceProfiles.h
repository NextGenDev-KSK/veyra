#pragma once

// Per-output-device memory: the service remembers the Config last used with each
// audio endpoint (keyed by a stable device id) and recalls it when that device
// becomes active again. Persisted as a single JSON file.

#include <filesystem>
#include <map>
#include <optional>
#include <string>

#include "veyra/Config.h"

namespace veyra {

class DeviceProfiles {
public:
    void remember(const std::string& deviceId, const Config& c);
    std::optional<Config> recall(const std::string& deviceId) const;
    bool   has(const std::string& deviceId) const;
    size_t size() const noexcept { return profiles_.size(); }

    std::string toJson() const;
    static DeviceProfiles fromJson(const std::string& text);

    bool save(const std::filesystem::path& file) const; // atomic write
    static DeviceProfiles load(const std::filesystem::path& file);

private:
    std::map<std::string, Config> profiles_;
};

} // namespace veyra
