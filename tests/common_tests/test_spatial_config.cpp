#include <catch2/catch.hpp>

#include "veyra/Config.h"

using namespace veyra;

TEST_CASE("Config: the spatial block round-trips through JSON")
{
    Config c;
    c.spatial.enabled   = true;
    c.spatial.crossfeed = 0.65f;
    c.spatial.mode      = 1;

    auto rt = Config::fromJson(c.toJson());
    REQUIRE(rt.has_value());
    CHECK(rt->spatial.enabled == true);
    CHECK(rt->spatial.crossfeed == Approx(0.65f));
    CHECK(rt->spatial.mode == 1);
}

TEST_CASE("Config: missing spatial block keeps defaults (disabled)")
{
    auto rt = Config::fromJson(R"({"version":1})");
    REQUIRE(rt.has_value());
    CHECK(rt->spatial.enabled == false);
    CHECK(rt->spatial.crossfeed == Approx(0.0f));
}
