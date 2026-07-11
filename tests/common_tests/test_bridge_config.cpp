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

TEST_CASE("Config: the mic_bridge block round-trips through JSON")
{
    Config c;
    c.micBridge.enabled = true;
    c.micBridge.micDeviceId = "{0.0.1.00000000}.{usb-mic-guid}";
    c.micBridge.targetDeviceId = "{0.0.0.00000000}.{cable-a-guid}";

    auto rt = Config::fromJson(c.toJson());
    REQUIRE(rt.has_value());
    CHECK(rt->micBridge.enabled == true);
    CHECK(rt->micBridge.micDeviceId == "{0.0.1.00000000}.{usb-mic-guid}");
    CHECK(rt->micBridge.targetDeviceId == "{0.0.0.00000000}.{cable-a-guid}");
}

TEST_CASE("Config: missing mic_bridge block keeps defaults")
{
    auto rt = Config::fromJson(R"({"version":1})");
    REQUIRE(rt.has_value());
    CHECK(rt->micBridge.enabled == false);
    CHECK(rt->micBridge.micDeviceId.empty());
    CHECK(rt->micBridge.targetDeviceId.empty());
}
