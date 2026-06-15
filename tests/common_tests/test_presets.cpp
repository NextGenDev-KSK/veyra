#include <catch2/catch.hpp>

#include <algorithm>
#include <string>
#include <vector>

#include "veyra/Config.h"
#include "veyra/Preset.h"

using namespace veyra;

TEST_CASE("Preset: JSON round-trips all fields")
{
    Preset p;
    p.uuid     = "u-123";
    p.name     = "My Preset";
    p.category = "Music";
    p.author   = "Tester";
    p.builtIn  = false;
    p.masterVolumeGain = 1.25;
    p.enhancement.bassBoostDb      = 6.0f;
    p.enhancement.trebleDb         = -2.0f;
    p.enhancement.volumeGain       = 1.5f;
    p.enhancement.stereoWidth      = 1.4f;
    p.enhancement.compressionAmount = 0.3f;
    p.enhancement.eqBandsDb[2]     = 4.0f;
    p.enhancement.eqBandsDb[7]     = -3.0f;

    auto rt = Preset::fromJson(p.toJson());
    REQUIRE(rt.has_value());
    CHECK(rt->uuid == "u-123");
    CHECK(rt->name == "My Preset");
    CHECK(rt->category == "Music");
    CHECK(rt->masterVolumeGain == Approx(1.25));
    CHECK(rt->enhancement.bassBoostDb == Approx(6.0f));
    CHECK(rt->enhancement.trebleDb == Approx(-2.0f));
    CHECK(rt->enhancement.stereoWidth == Approx(1.4f));
    CHECK(rt->enhancement.compressionAmount == Approx(0.3f));
    CHECK(rt->enhancement.eqBandsDb[2] == Approx(4.0f));
    CHECK(rt->enhancement.eqBandsDb[7] == Approx(-3.0f));
}

TEST_CASE("Preset: parsing rejects documents without an identity")
{
    CHECK_FALSE(Preset::fromJson("{}").has_value());
    CHECK_FALSE(Preset::fromJson("not json").has_value());
    CHECK_FALSE(Preset::fromJson(R"({"name":"x"})").has_value()); // no uuid
}

TEST_CASE("Preset: applyTo writes gains, enhancement and active uuid")
{
    Config c;
    c.activePresetUuid = "old";

    Preset p;
    p.uuid = "v-bass-boost";
    p.name = "Bass";
    p.masterVolumeGain = 0.8;
    p.enhancement.bassBoostDb = 5.0f;

    p.applyTo(c);
    CHECK(c.activePresetUuid == "v-bass-boost");
    CHECK(c.masterVolumeGain == Approx(0.8));
    CHECK(c.enhancement.bassBoostDb == Approx(5.0f));
}

TEST_CASE("Built-in presets are present, identifiable and unique")
{
    const auto& presets = builtInPresets();
    REQUIRE(presets.size() >= 6);

    std::vector<std::string> seen;
    for (const auto& p : presets)
    {
        CHECK(p.builtIn);
        CHECK_FALSE(p.uuid.empty());
        CHECK_FALSE(p.name.empty());
        CHECK(std::find(seen.begin(), seen.end(), p.uuid) == seen.end());
        seen.push_back(p.uuid);

        // Every built-in must survive a JSON round-trip.
        auto rt = Preset::fromJson(p.toJson());
        REQUIRE(rt.has_value());
        CHECK(rt->uuid == p.uuid);
    }
}
