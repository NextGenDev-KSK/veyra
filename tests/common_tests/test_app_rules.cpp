#include <catch2/catch.hpp>

#include "veyra/AppRules.h"
#include "veyra/RateLimiter.h"

using namespace veyra;

TEST_CASE("AppRuleEngine: resolves by case-insensitive substring")
{
    AppRuleEngine eng;
    eng.setRules({
        {"chrome.exe", "v-flat", 0, true},
        {"valorant",   "v-fps-footsteps", 10, true},
    });

    CHECK(eng.resolve("C:/Games/VALORANT-Win64-Shipping.exe") == "v-fps-footsteps");
    CHECK(eng.resolve("chrome.EXE") == "v-flat");
    CHECK_FALSE(eng.resolve("explorer.exe").has_value());
}

TEST_CASE("AppRuleEngine: highest priority wins on overlap")
{
    AppRuleEngine eng;
    eng.setRules({
        {"game", "v-flat", 1, true},
        {"game", "v-cinematic", 5, true},
        {"game", "v-night", 3, true},
    });
    CHECK(eng.resolve("supergame.exe") == "v-cinematic");
}

TEST_CASE("AppRuleEngine: disabled and empty rules are ignored")
{
    AppRuleEngine eng;
    eng.setRules({
        {"game", "v-cinematic", 9, false}, // disabled despite high priority
        {"game", "v-flat", 1, true},
        {"",     "v-night", 99, true},      // empty match never applies
    });
    CHECK(eng.resolve("game.exe") == "v-flat");
}

TEST_CASE("AppRuleEngine: rules survive a JSON round-trip")
{
    AppRuleEngine eng;
    eng.setRules({
        {"valorant", "v-fps-footsteps", 10, true},
        {"spotify",  "v-bass-boost", 2, false},
    });

    auto parsed = AppRuleEngine::rulesFromJson(eng.toJson());
    REQUIRE(parsed.size() == 2);
    CHECK(parsed[0].match == "valorant");
    CHECK(parsed[0].presetUuid == "v-fps-footsteps");
    CHECK(parsed[0].priority == 10);
    CHECK(parsed[0].enabled == true);
    CHECK(parsed[1].enabled == false);
}

TEST_CASE("RateLimiter: gates calls within the minimum interval")
{
    using namespace std::chrono;
    RateLimiter rl(milliseconds(500));
    const RateLimiter::TimePoint t0{};

    CHECK(rl.allow(t0));                              // first is always allowed
    CHECK_FALSE(rl.allow(t0 + milliseconds(100)));   // too soon
    CHECK_FALSE(rl.allow(t0 + milliseconds(499)));
    CHECK(rl.allow(t0 + milliseconds(500)));         // interval elapsed
    CHECK_FALSE(rl.allow(t0 + milliseconds(800)));
    CHECK(rl.allow(t0 + milliseconds(1000)));
}
