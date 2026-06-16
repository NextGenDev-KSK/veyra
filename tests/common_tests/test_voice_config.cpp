#include <catch2/catch.hpp>

#include "veyra/Config.h"

using namespace veyra;

TEST_CASE("Config: the voice (mic) block round-trips through JSON")
{
    Config c;
    c.voice.enabled           = false;
    c.voice.highPassHz        = 120.0f;
    c.voice.noiseSuppression  = 0.8f;
    c.voice.compressionAmount = 0.4f;
    c.voice.deEssAmount       = 0.6f;
    c.voice.presenceDb        = 4.0f;
    c.voice.outputGainDb      = -2.0f;
    c.voice.sideToneLevel     = 0.5f;

    auto rt = Config::fromJson(c.toJson());
    REQUIRE(rt.has_value());
    CHECK(rt->voice.enabled == false);
    CHECK(rt->voice.highPassHz == Approx(120.0f));
    CHECK(rt->voice.noiseSuppression == Approx(0.8f));
    CHECK(rt->voice.compressionAmount == Approx(0.4f));
    CHECK(rt->voice.deEssAmount == Approx(0.6f));
    CHECK(rt->voice.presenceDb == Approx(4.0f));
    CHECK(rt->voice.outputGainDb == Approx(-2.0f));
    CHECK(rt->voice.sideToneLevel == Approx(0.5f));
}

TEST_CASE("Config: missing voice block keeps defaults")
{
    auto rt = Config::fromJson(R"({"version":1})");
    REQUIRE(rt.has_value());
    CHECK(rt->voice.enabled == true);
    CHECK(rt->voice.noiseSuppression == Approx(0.5f));
}
