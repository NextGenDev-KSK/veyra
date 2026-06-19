#include <catch2/catch.hpp>

#include "veyra/Config.h"

using namespace veyra;

TEST_CASE("Config: the bridge block round-trips through JSON")
{
    Config c;
    c.bridge.enabled = true;
    c.bridge.sourceDeviceId = "{0.0.0.00000000}.{virtual-sink-guid}";
    c.bridge.targetDeviceId = "{0.0.0.00000000}.{nord-buds-guid}";

    auto rt = Config::fromJson(c.toJson());
    REQUIRE(rt.has_value());
    CHECK(rt->bridge.enabled == true);
    CHECK(rt->bridge.sourceDeviceId == "{0.0.0.00000000}.{virtual-sink-guid}");
    CHECK(rt->bridge.targetDeviceId == "{0.0.0.00000000}.{nord-buds-guid}");
}

TEST_CASE("Config: missing bridge block keeps defaults")
{
    auto rt = Config::fromJson(R"({"version":1})");
    REQUIRE(rt.has_value());
    CHECK(rt->bridge.enabled == false);
    CHECK(rt->bridge.sourceDeviceId.empty());
}
