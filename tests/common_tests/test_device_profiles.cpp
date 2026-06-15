#include <catch2/catch.hpp>

#include "veyra/Config.h"
#include "veyra/DeviceProfiles.h"

using namespace veyra;

TEST_CASE("DeviceProfiles: remember / recall by device id")
{
    DeviceProfiles dp;
    CHECK_FALSE(dp.recall("speakers").has_value());

    Config c;
    c.masterVolumeGain = 0.7;
    c.enhancement.bassBoostDb = 4.0f;
    dp.remember("speakers", c);

    CHECK(dp.has("speakers"));
    auto r = dp.recall("speakers");
    REQUIRE(r.has_value());
    CHECK(r->masterVolumeGain == Approx(0.7));
    CHECK(r->enhancement.bassBoostDb == Approx(4.0f));
}

TEST_CASE("DeviceProfiles: empty device id is not stored")
{
    DeviceProfiles dp;
    dp.remember("", Config{});
    CHECK(dp.size() == 0);
}

TEST_CASE("DeviceProfiles: JSON round-trips multiple devices")
{
    DeviceProfiles dp;

    Config a;
    a.masterVolumeGain = 1.2;
    a.enhancement.trebleDb = 3.0f;
    dp.remember("headphones", a);

    Config b;
    b.masterEnabled = false;
    b.enhancement.stereoWidth = 1.5f;
    dp.remember("speakers", b);

    auto rt = DeviceProfiles::fromJson(dp.toJson());
    REQUIRE(rt.size() == 2);

    auto h = rt.recall("headphones");
    REQUIRE(h.has_value());
    CHECK(h->masterVolumeGain == Approx(1.2));
    CHECK(h->enhancement.trebleDb == Approx(3.0f));

    auto s = rt.recall("speakers");
    REQUIRE(s.has_value());
    CHECK(s->masterEnabled == false);
    CHECK(s->enhancement.stereoWidth == Approx(1.5f));
}
