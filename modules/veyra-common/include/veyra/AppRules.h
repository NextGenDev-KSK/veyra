#pragma once

// Per-application rules: map a foreground app (by a case-insensitive substring
// of its process/exe id) to a preset. The service resolves the active preset
// for whatever app currently has focus; the actual focus tracking and rate
// limiting live in the service, but the resolution logic is here so it can be
// unit-tested without Windows.

#include <optional>
#include <string>
#include <vector>

namespace veyra {

struct AppRule {
    std::string match;            // matched as a lowercase substring of the app id
    std::string presetUuid;       // preset to apply when this rule wins
    int         priority = 0;     // higher wins; ties resolve to the earlier rule
    bool        enabled  = true;
    std::string detection = "foreground"; // foreground | audio | both
    float       volume    = 1.0f;         // per-app volume (0..1.5)
    bool        autoMute  = false;        // mute this app when it loses focus
};

class AppRuleEngine {
public:
    void setRules(std::vector<AppRule> rules) { rules_ = std::move(rules); }
    const std::vector<AppRule>& rules() const noexcept { return rules_; }

    // Highest-priority enabled rule whose 'match' is contained in 'appId'
    // (case-insensitive). Returns the preset uuid, or nullopt if none match.
    std::optional<std::string> resolve(const std::string& appId) const;

    // JSON array form for IPC / persistence.
    std::string toJson() const;
    static std::vector<AppRule> rulesFromJson(const std::string& text);

private:
    std::vector<AppRule> rules_;
};

} // namespace veyra
