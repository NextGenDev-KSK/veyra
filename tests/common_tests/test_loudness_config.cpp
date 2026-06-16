#include <catch2/catch.hpp>

#include "veyra/Config.h"

using namespace veyra;

TEST_CASE("Config: the loudness block round-trips through JSON")
{
    Config c;
    c.loudness.nightModeAmount   = 0.7f;
    c.loudness.sleepTimerEnabled = true;
    c.loudness.sleepTimerMinutes = 45.0f;
    c.loudness.sleepFadeSeconds  = 30.0f;

    auto rt = Config::fromJson(c.toJson());
    REQUIRE(rt.has_value());
    CHECK(rt->loudness.nightModeAmount == Approx(0.7f));
    CHECK(rt->loudness.sleepTimerEnabled == true);
    CHECK(rt->loudness.sleepTimerMinutes == Approx(45.0f));
    CHECK(rt->loudness.sleepFadeSeconds == Approx(30.0f));
}

TEST_CASE("Config: missing loudness block keeps defaults")
{
    auto rt = Config::fromJson(R"({"version":1})");
    REQUIRE(rt.has_value());
    CHECK(rt->loudness.nightModeAmount == Approx(0.0f));
    CHECK(rt->loudness.sleepTimerEnabled == false);
}
