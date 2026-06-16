#include <catch2/catch.hpp>

#include "veyra/Config.h"

using namespace veyra;

TEST_CASE("Config: appearance (theme/opacity/background/reduce-motion) round-trips")
{
    Config c;
    c.theme          = "aurora";
    c.uiOpacity      = 0.45;
    c.backgroundMode = 2;
    c.reduceMotion   = true;

    auto rt = Config::fromJson(c.toJson());
    REQUIRE(rt.has_value());
    CHECK(rt->theme == "aurora");
    CHECK(rt->uiOpacity == Approx(0.45));
    CHECK(rt->backgroundMode == 2);
    CHECK(rt->reduceMotion == true);
}

TEST_CASE("Config: missing appearance keys keep defaults")
{
    auto rt = Config::fromJson(R"({"appearance":{"theme":"midnight"}})");
    REQUIRE(rt.has_value());
    CHECK(rt->theme == "midnight");
    CHECK(rt->uiOpacity == Approx(0.85));
    CHECK(rt->backgroundMode == 0);
    CHECK(rt->reduceMotion == false);
}
