#pragma once

// Owns the available presets: the built-in set plus user `.vpreset` files under
// %APPDATA%\Veyra\presets. Thread-safe (the control server reads/writes it from
// the pipe listener thread).

#include <filesystem>
#include <mutex>
#include <optional>
#include <string>
#include <vector>

#include "veyra/Preset.h"

namespace veyra {
class Logger;
}

namespace veyra::service {

class PresetLibrary {
public:
    PresetLibrary(std::filesystem::path dir, Logger* log);

    // Load built-ins and (re)scan the user preset directory.
    void load();

    // Built-ins first, then user presets.
    std::vector<Preset> all() const;
    std::optional<Preset> find(const std::string& uuid) const;

    // Persist a user preset (forces builtIn=false); refuses to overwrite a
    // built-in uuid. Returns false on I/O failure or an invalid uuid.
    bool saveUser(const Preset& preset);

    // Delete a user preset by uuid. Built-ins cannot be deleted.
    bool removeUser(const std::string& uuid);

    // All presets as a JSON array (for the ListPresets reply).
    std::string toJsonArray() const;

private:
    std::filesystem::path fileFor(const std::string& uuid) const;
    bool isBuiltIn(const std::string& uuid) const;

    std::filesystem::path dir_;
    Logger*               log_;
    std::vector<Preset>   user_;
    mutable std::mutex    mutex_;
};

} // namespace veyra::service
