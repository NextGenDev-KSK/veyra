#include "veyra/AppRules.h"

#include <algorithm>
#include <cctype>

#include <nlohmann/json.hpp>

namespace veyra {

using nlohmann::json;

namespace {
std::string toLower(std::string s)
{
    std::transform(s.begin(), s.end(), s.begin(),
                   [](unsigned char c) { return (char) std::tolower(c); });
    return s;
}
} // namespace

std::optional<std::string> AppRuleEngine::resolve(const std::string& appId) const
{
    const std::string hay = toLower(appId);
    const AppRule* best = nullptr;
    for (const auto& r : rules_)
    {
        if (!r.enabled || r.match.empty())
            continue;
        if (hay.find(toLower(r.match)) == std::string::npos)
            continue;
        if (best == nullptr || r.priority > best->priority)
            best = &r;
    }
    if (best == nullptr)
        return std::nullopt;
    return best->presetUuid;
}

std::string AppRuleEngine::toJson() const
{
    json arr = json::array();
    for (const auto& r : rules_)
        arr.push_back({{"match", r.match},
                       {"preset_uuid", r.presetUuid},
                       {"priority", r.priority},
                       {"enabled", r.enabled}});
    return arr.dump(2);
}

std::vector<AppRule> AppRuleEngine::rulesFromJson(const std::string& text)
{
    std::vector<AppRule> out;
    json j = json::parse(text, nullptr, /*allow_exceptions=*/false);
    if (j.is_discarded() || !j.is_array())
        return out;

    for (const auto& e : j)
    {
        if (!e.is_object())
            continue;
        AppRule r;
        r.match      = e.value("match", std::string{});
        r.presetUuid = e.value("preset_uuid", std::string{});
        r.priority   = e.value("priority", 0);
        r.enabled    = e.value("enabled", true);
        if (!r.match.empty() && !r.presetUuid.empty())
            out.push_back(std::move(r));
    }
    return out;
}

} // namespace veyra
