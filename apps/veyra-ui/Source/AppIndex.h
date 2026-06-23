#pragma once

// Offline installed-application index for the Apps page. Scans the Start Menu
// (.lnk shortcuts -> resolved .exe targets) and extracts each app's icon
// straight from the EXE — no internet, no GitHub, no third-party services.

#include "VeyraGui.h"

#include <vector>

namespace veyra::ui {

struct InstalledApp {
    juce::String name;  // friendly name (shortcut name)
    juce::String match; // lowercased exe base name (used as the per-app rule match)
    juce::Image  icon;  // extracted from the EXE (may be invalid on failure)
};

// Enumerate installed apps from the common + per-user Start Menu. Sorted by name,
// de-duplicated by exe. Best-effort; returns what it can.
std::vector<InstalledApp> scanInstalledApps();

// Enumerate currently running user applications (skips Windows-dir/system
// processes), de-duplicated by exe, with icons extracted from the process image.
// Lets the picker offer apps that aren't in the Start Menu but are open now.
std::vector<InstalledApp> scanRunningProcesses();

// The Veyra brand mark, used as the icon for an app we couldn't extract one for
// (so a row never shows a blank icon). Cached after first build.
juce::Image defaultAppIcon();

} // namespace veyra::ui
