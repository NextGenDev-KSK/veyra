#pragma once

// Update-check logic: semantic-version parsing/comparison and evaluation of a
// release feed against the current version. The feed is either a simple manifest
// object { "version", "url", "notes" } or a GitHub-releases array (tag_name /
// html_url / body, skipping draft + prerelease). The actual HTTPS fetch,
// download and apply are runtime (service-side); this is the pure, testable
// decision core. JSON stays out of this header.

#include <array>
#include <cctype>
#include <optional>
#include <sstream>
#include <string>

namespace veyra {

struct Version {
    int major = 0, minor = 0, patch = 0;

    static std::optional<Version> parse(const std::string& in)
    {
        if (in.empty())
            return std::nullopt;
        std::string s = in;
        if (s[0] == 'v' || s[0] == 'V')
            s.erase(0, 1);
        if (const auto cut = s.find_first_of("-+"); cut != std::string::npos)
            s = s.substr(0, cut); // drop pre-release / build metadata

        std::array<int, 3> parts{0, 0, 0};
        std::stringstream ss(s);
        std::string tok;
        int n = 0;
        bool any = false;
        while (std::getline(ss, tok, '.') && n < 3)
        {
            if (tok.empty())
                return std::nullopt;
            for (char c : tok)
                if (!std::isdigit((unsigned char) c))
                    return std::nullopt;
            parts[(size_t) n++] = std::stoi(tok);
            any = true;
        }
        if (!any)
            return std::nullopt;
        return Version{parts[0], parts[1], parts[2]};
    }

    std::string toString() const
    {
        return std::to_string(major) + "." + std::to_string(minor) + "." + std::to_string(patch);
    }

    bool operator<(const Version& o) const
    {
        if (major != o.major) return major < o.major;
        if (minor != o.minor) return minor < o.minor;
        return patch < o.patch;
    }
    bool operator==(const Version& o) const
    {
        return major == o.major && minor == o.minor && patch == o.patch;
    }
};

struct UpdateInfo {
    bool        available = false;
    std::string version; // canonical "major.minor.patch" of the newest release
    std::string url;
    std::string notes;
};

struct UpdateChecker {
    // currentVersion e.g. "0.3.0"; feedJson = manifest object or releases array.
    static UpdateInfo evaluate(const std::string& currentVersion, const std::string& feedJson);
};

} // namespace veyra
