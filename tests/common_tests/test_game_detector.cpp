#include <catch2/catch.hpp>

#include "veyra/GameDetector.h"

using namespace veyra;

TEST_CASE("GameDetector: matches known games by exe substring (case-insensitive)")
{
    GameDetector d;
    CHECK(d.isGame("C:\\Riot Games\\VALORANT\\live\\VALORANT-Win64-Shipping.exe"));
    CHECK(d.isGame("cs2.exe"));
    CHECK(d.isGame("D:\\Games\\RocketLeague.exe"));
    CHECK(d.isGame("Apex Legends\\r5apex.exe"));
}

TEST_CASE("GameDetector: ignores non-games and empty input")
{
    GameDetector d;
    CHECK_FALSE(d.isGame("chrome.exe"));
    CHECK_FALSE(d.isGame("explorer.exe"));
    CHECK_FALSE(d.isGame("veyra.exe"));
    CHECK_FALSE(d.isGame(""));
}

TEST_CASE("GameDetector: custom list overrides the built-ins")
{
    GameDetector d;
    d.setGames({"mygame"});
    CHECK(d.isGame("MyGame.exe"));
    CHECK_FALSE(d.isGame("valorant.exe")); // no longer in the list
    CHECK(d.games().size() == 1);
}
