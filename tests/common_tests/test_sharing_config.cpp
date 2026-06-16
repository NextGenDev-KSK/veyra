#include <catch2/catch.hpp>

#include "veyra/Config.h"

using namespace veyra;

TEST_CASE("Config: the sharing block round-trips a list of routes")
{
    Config c;
    c.sharing.enabled = true;

    OutputRoute a;
    a.deviceId = "dev-speakers";
    a.name = "Living Room";
    a.primary = true;
    a.gainDb = 0.0f;
    a.delayMs = 0.0f;
    c.sharing.routes.push_back(a);

    OutputRoute b;
    b.deviceId = "dev-bt";
    b.name = "Kitchen BT";
    b.enabled = true;
    b.gainDb = -3.0f;
    b.delayMs = 120.0f; // bluetooth lag compensation
    c.sharing.routes.push_back(b);

    auto rt = Config::fromJson(c.toJson());
    REQUIRE(rt.has_value());
    CHECK(rt->sharing.enabled == true);
    REQUIRE(rt->sharing.routes.size() == 2);

    CHECK(rt->sharing.routes[0].deviceId == "dev-speakers");
    CHECK(rt->sharing.routes[0].primary == true);

    CHECK(rt->sharing.routes[1].deviceId == "dev-bt");
    CHECK(rt->sharing.routes[1].name == "Kitchen BT");
    CHECK(rt->sharing.routes[1].gainDb == Approx(-3.0f));
    CHECK(rt->sharing.routes[1].delayMs == Approx(120.0f));
}

TEST_CASE("Config: routes without a device id are dropped; defaults preserved")
{
    auto rt = Config::fromJson(R"({"sharing":{"enabled":true,"routes":[{"name":"no id"}]}})");
    REQUIRE(rt.has_value());
    CHECK(rt->sharing.enabled == true);
    CHECK(rt->sharing.routes.empty());

    auto def = Config::fromJson(R"({"version":1})");
    REQUIRE(def.has_value());
    CHECK(def->sharing.enabled == false);
    CHECK(def->sharing.routes.empty());
}
