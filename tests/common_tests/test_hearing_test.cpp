#include <catch2/catch.hpp>

#include <vector>

#include "veyra/HearingTest.h"

using namespace veyra;

TEST_CASE("personalizeFromHearingTest applies the half-gain rule with a cap")
{
    std::vector<HearingPoint> pts = {
        {1000.0f, 0.0f},   // normal -> no band
        {4000.0f, 20.0f},  // 20 dB loss -> +10 dB
        {8000.0f, 30.0f},  // 30 dB loss -> +15 -> capped to +12
        {500.0f, 0.4f},    // negligible -> dropped
    };
    const auto bands = personalizeFromHearingTest(pts);

    REQUIRE(bands.size() == 2);          // only 4 k and 8 k qualify
    CHECK(bands[0].freq == Approx(4000.0f)); // sorted by frequency
    CHECK(bands[0].gainDb == Approx(10.0f));
    CHECK(bands[1].freq == Approx(8000.0f));
    CHECK(bands[1].gainDb == Approx(12.0f)); // capped
    for (const auto& b : bands) { CHECK(b.enabled); CHECK(b.type == 0); }
}

TEST_CASE("personalizeFromHearingTest: normal hearing yields a flat (empty) curve")
{
    std::vector<HearingPoint> pts = {{250.0f, 0.0f}, {1000.0f, 0.0f}, {4000.0f, 0.0f}};
    CHECK(personalizeFromHearingTest(pts).empty());
}
