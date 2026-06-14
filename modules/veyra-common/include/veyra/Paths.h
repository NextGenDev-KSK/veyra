#pragma once

// Canonical on-disk locations for Veyra state, all under %APPDATA%\Veyra.
// (For the LocalSystem service this resolves to the system profile's roaming
// AppData, which is the intended behaviour for service-owned config + logs.)

#include <filesystem>

namespace veyra::paths {

// %APPDATA%\Veyra
std::filesystem::path appDataDir();

// %APPDATA%\Veyra\logs
std::filesystem::path logsDir();

// %APPDATA%\Veyra\config.json
std::filesystem::path configFile();

// %APPDATA%\Veyra\presets
std::filesystem::path presetsDir();

// %APPDATA%\Veyra\crashes
std::filesystem::path crashesDir();

// Create a directory (and parents) if missing. Best-effort; ignores errors.
void ensureDir(const std::filesystem::path& dir);

} // namespace veyra::paths
