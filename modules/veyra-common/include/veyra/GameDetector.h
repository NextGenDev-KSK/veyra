#pragma once

// Decides whether a foreground application is a game, by a case-insensitive
// substring match of its process/exe id against a list. Pure + testable; the
// service does the actual foreground polling. Anti-cheat safe — name only, never
// a graphics-API hook.

#include <string>
#include <vector>

namespace veyra {

class GameDetector {
public:
    GameDetector(); // seeded with the built-in list

    void setGames(std::vector<std::string> names);
    const std::vector<std::string>& games() const noexcept { return games_; }

    // True if any list entry is a lowercase substring of appId.
    bool isGame(const std::string& appId) const;

private:
    std::vector<std::string> games_;
};

// The bundled list of common game executables (lowercase).
const std::vector<std::string>& builtInGameList();

} // namespace veyra
