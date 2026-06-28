#include "veyra/Paths.h"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <shlobj.h>

#include <system_error>

namespace veyra::paths {

namespace {

// Use ProgramData (%ProgramData%) so both the installed LocalSystem service
// and the interactive user process (UI, overlay) resolve the SAME directory.
// %AppData% is per-user-session; LocalSystem resolves it to
// C:\Windows\System32\config\systemprofile\AppData, not the user's profile.
// ProgramData is machine-wide, writable by any authenticated user by default,
// and is the standard location for shared machine-scope application data on
// Windows (same pattern used by Realtek, DTS, Dolby audio middleware).
std::filesystem::path programData()
{
    PWSTR raw = nullptr;
    if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_ProgramData, 0, nullptr, &raw)))
    {
        std::filesystem::path p{raw};
        CoTaskMemFree(raw);
        return p;
    }
    return std::filesystem::temp_directory_path();
}

} // namespace

std::filesystem::path appDataDir()
{
    return programData() / L"Veyra";
}

std::filesystem::path logsDir()    { return appDataDir() / L"logs"; }
std::filesystem::path configFile() { return appDataDir() / L"config.json"; }
std::filesystem::path presetsDir() { return appDataDir() / L"presets"; }
std::filesystem::path crashesDir() { return appDataDir() / L"crashes"; }

void ensureDir(const std::filesystem::path& dir)
{
    std::error_code ec;
    std::filesystem::create_directories(dir, ec);
}

} // namespace veyra::paths
