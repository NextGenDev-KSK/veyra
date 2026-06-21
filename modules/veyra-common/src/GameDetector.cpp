#include "veyra/GameDetector.h"

#include <algorithm>
#include <cctype>

namespace veyra {

namespace {
std::string toLower(std::string s)
{
    std::transform(s.begin(), s.end(), s.begin(),
                   [](unsigned char c) { return (char) std::tolower(c); });
    return s;
}
} // namespace

const std::vector<std::string>& builtInGameList()
{
    // Common competitive + popular titles, matched as substrings of the exe id.
    static const std::vector<std::string> kGames = {
        "valorant", "csgo", "cs2", "fortnite", "fortniteclient", "apex", "r5apex",
        "overwatch", "pubg", "tslgame", "rainbowsix", "destiny2", "modernwarfare",
        "warzone", "cod", "dota2", "leagueoflegends", "league of legends", "rocketleague",
        "gta5", "gtav", "minecraft", "battlefield", "bf2042", "escapefromtarkov", "eft",
        "halo", "fallguys", "thefinals", "marvelrivals", "deadlock",
    };
    return kGames;
}

GameDetector::GameDetector() : games_(builtInGameList()) {}

void GameDetector::setGames(std::vector<std::string> names) { games_ = std::move(names); }

bool GameDetector::isGame(const std::string& appId) const
{
    const std::string hay = toLower(appId);
    if (hay.empty())
        return false;
    for (const auto& g : games_)
        if (!g.empty() && hay.find(g) != std::string::npos)
            return true;
    return false;
}

} // namespace veyra
