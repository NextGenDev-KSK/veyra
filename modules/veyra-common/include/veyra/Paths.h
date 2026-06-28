#pragma once

// Canonical on-disk locations for Veyra state, all under %ProgramData%\Veyra
// (C:\ProgramData\Veyra).  Using ProgramData instead of %AppData% lets both the
// installed LocalSystem service and the interactive user process (UI, overlay)
// resolve the same directory regardless of which Windows account runs each binary.
// ProgramData is machine-wide and writable by Authenticated Users by default.

#include <filesystem>

namespace veyra::paths {

// C:\ProgramData\Veyra
std::filesystem::path appDataDir();

// C:\ProgramData\Veyra\logs
std::filesystem::path logsDir();

// C:\ProgramData\Veyra\config.json
std::filesystem::path configFile();

// C:\ProgramData\Veyra\presets
std::filesystem::path presetsDir();

// C:\ProgramData\Veyra\crashes
std::filesystem::path crashesDir();

// Create a directory (and parents) if missing. Best-effort; ignores errors.
void ensureDir(const std::filesystem::path& dir);

} // namespace veyra::paths
