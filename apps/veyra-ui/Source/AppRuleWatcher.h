#pragma once

// Per-app rule watcher (user session). Polls the foreground application and,
// when it changes, resolves a preset from the rules the service persists
// (app_rules.json) via the shared, tested veyra::AppRuleEngine. Returns the
// preset uuid to apply when a *different* rule wins, so the shell can push a
// LoadPreset. Foreground tracking has to live here, not in the LocalSystem
// service, which runs in session 0 and can't see the user's desktop.

#include <filesystem>
#include <optional>
#include <string>

#include "veyra/AppRules.h"

namespace veyra::ui {

class AppRuleWatcher {
public:
    void setRulesFile(std::filesystem::path file);

    // Call periodically (~1 Hz). Returns a preset uuid to apply, or nullopt.
    std::optional<std::string> poll();

private:
    void reloadIfChanged();

    veyra::AppRuleEngine            engine_;
    std::filesystem::path           file_;
    std::filesystem::file_time_type mtime_{};
    bool                            haveMtime_ = false;
    std::string                     lastApp_, lastApplied_;
};

} // namespace veyra::ui
