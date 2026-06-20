#include "veyra/Updater.h"

#include <nlohmann/json.hpp>

namespace veyra {

using nlohmann::json;

UpdateInfo UpdateChecker::evaluate(const std::string& currentVersion, const std::string& feedJson)
{
    UpdateInfo info;

    const auto cur = Version::parse(currentVersion);
    if (!cur)
        return info;

    json j = json::parse(feedJson, nullptr, /*allow_exceptions=*/false);
    if (j.is_discarded())
        return info;

    std::optional<Version> best;
    std::string bestUrl, bestNotes;

    auto offer = [&](const Version& v, const std::string& url, const std::string& notes)
    {
        if (!(*cur < v))      // only releases strictly newer than current
            return;
        if (!best || *best < v)
        {
            best = v;
            bestUrl = url;
            bestNotes = notes;
        }
    };

    if (j.is_array()) // GitHub-releases style
    {
        for (const auto& e : j)
        {
            if (!e.is_object())
                continue;
            if (e.value("draft", false) || e.value("prerelease", false))
                continue;
            if (auto v = Version::parse(e.value("tag_name", std::string{})))
                offer(*v, e.value("html_url", std::string{}), e.value("body", std::string{}));
        }
    }
    else if (j.is_object()) // simple manifest
    {
        if (auto v = Version::parse(j.value("version", std::string{})))
            offer(*v, j.value("url", std::string{}), j.value("notes", std::string{}));
    }

    if (best)
    {
        info.available = true;
        info.version = best->toString();
        info.url = bestUrl;
        info.notes = bestNotes;
    }
    return info;
}

} // namespace veyra
