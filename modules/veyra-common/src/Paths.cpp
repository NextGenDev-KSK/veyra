#include "veyra/Paths.h"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <shlobj.h>

#include <system_error>

namespace veyra::paths {

namespace {

std::filesystem::path roamingAppData()
{
    PWSTR raw = nullptr;
    if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_RoamingAppData, 0, nullptr, &raw)))
    {
        std::filesystem::path p{raw};
        CoTaskMemFree(raw);
        return p;
    }
    // Extremely unlikely fallback.
    return std::filesystem::temp_directory_path();
}

} // namespace

std::filesystem::path appDataDir()
{
    return roamingAppData() / L"Veyra";
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
