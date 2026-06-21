#include <catch2/catch.hpp>

#include <algorithm>

#include "veyra/AutoEq.h"

using namespace veyra;

TEST_CASE("AutoEq: parses preamp + filters into ParametricBands")
{
    const std::string hd600 =
        "Preamp: -6.3 dB\n"
        "Filter 1: ON LSC Fc 105 Hz Gain 6.5 dB Q 0.70\n"
        "Filter 2: ON PK Fc 125 Hz Gain -2.7 dB Q 0.55\n"
        "Filter 6: ON HSC Fc 10000 Hz Gain -3.1 dB Q 0.70\n";

    auto p = parseAutoEq("Sennheiser HD 600", hd600);
    REQUIRE(p.has_value());
    CHECK(p->name == "Sennheiser HD 600");
    CHECK(p->preampDb == Approx(-6.3f));
    REQUIRE(p->bands.size() == 3);
    CHECK(p->bands[0].type == 1);              // LSC -> low shelf
    CHECK(p->bands[0].freq == Approx(105.0f));
    CHECK(p->bands[0].gainDb == Approx(6.5f));
    CHECK(p->bands[0].q == Approx(0.70f));
    CHECK(p->bands[1].type == 0);              // PK -> bell
    CHECK(p->bands[2].type == 2);              // HSC -> high shelf
}

TEST_CASE("AutoEq: OFF filters and empty input yield no profile")
{
    CHECK_FALSE(parseAutoEq("x", "Filter 1: OFF PK Fc 100 Hz Gain 1 dB Q 1\n").has_value());
    CHECK_FALSE(parseAutoEq("x", "").has_value());
}

#ifdef VEYRA_AUTOEQ_DIR
TEST_CASE("AutoEq: the vendored bundle loads and parses")
{
    const auto profiles = loadAutoEqProfiles(VEYRA_AUTOEQ_DIR);
    REQUIRE(profiles.size() >= 16);
    for (const auto& p : profiles)
    {
        CHECK_FALSE(p.name.empty());
        CHECK_FALSE(p.bands.empty());
    }
    // sorted by name
    CHECK(std::is_sorted(profiles.begin(), profiles.end(),
                         [](const AutoEqProfile& a, const AutoEqProfile& b) { return a.name < b.name; }));
}
#endif
