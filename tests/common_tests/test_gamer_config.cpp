#include <catch2/catch.hpp>

#include "veyra/Config.h"

using namespace veyra;

TEST_CASE("Config: the gamer_mode block round-trips through JSON")
{
    Config c;
    c.gamerMode.enabled     = true;
    c.gamerMode.sensitivity = 0.85f;
    c.gamerMode.radarMode   = 2;
    c.gamerMode.footsteps   = true;
    c.gamerMode.gunshots    = false;
    c.gamerMode.voices      = true;

    auto rt = Config::fromJson(c.toJson());
    REQUIRE(rt.has_value());
    CHECK(rt->gamerMode.enabled == true);
    CHECK(rt->gamerMode.sensitivity == Approx(0.85f));
    CHECK(rt->gamerMode.radarMode == 2);
    CHECK(rt->gamerMode.footsteps == true);
    CHECK(rt->gamerMode.gunshots == false);
    CHECK(rt->gamerMode.voices == true);
}

TEST_CASE("Config: missing gamer_mode keeps defaults (disabled)")
{
    auto rt = Config::fromJson(R"({"version":1})");
    REQUIRE(rt.has_value());
    CHECK(rt->gamerMode.enabled == false);
    CHECK(rt->gamerMode.radarMode == 0);
}
