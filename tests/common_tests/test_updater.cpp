#include <catch2/catch.hpp>

#include "veyra/Updater.h"

using namespace veyra;

TEST_CASE("Version: parses and compares semantic versions")
{
    auto a = Version::parse("0.3.0");
    auto b = Version::parse("v1.2.3");
    REQUIRE(a.has_value());
    REQUIRE(b.has_value());
    CHECK(a->major == 0); CHECK(a->minor == 3); CHECK(a->patch == 0);
    CHECK(b->major == 1); CHECK(b->minor == 2); CHECK(b->patch == 3);

    CHECK(*Version::parse("0.4.0") < *Version::parse("0.10.0")); // numeric, not lexical
    CHECK(*Version::parse("0.9.9") < *Version::parse("1.0.0"));
    CHECK(*Version::parse("1.2.3") == *Version::parse("v1.2.3-beta+exp"));
    CHECK(Version::parse("not.a.version") == std::nullopt);
    CHECK(Version::parse("") == std::nullopt);
}

TEST_CASE("UpdateChecker: manifest object newer/older")
{
    const auto newer = UpdateChecker::evaluate(
        "0.3.0", R"({"version":"0.4.0","url":"https://x/y","notes":"stuff"})");
    CHECK(newer.available);
    CHECK(newer.version == "0.4.0");
    CHECK(newer.url == "https://x/y");
    CHECK(newer.notes == "stuff");

    CHECK_FALSE(UpdateChecker::evaluate("0.4.0", R"({"version":"0.4.0"})").available); // same
    CHECK_FALSE(UpdateChecker::evaluate("0.4.0", R"({"version":"0.3.9"})").available); // older
}

TEST_CASE("UpdateChecker: GitHub-releases array picks highest stable")
{
    const std::string feed = R"([
        {"tag_name":"v0.5.0-rc1","prerelease":true,"html_url":"u-rc"},
        {"tag_name":"v0.4.2","html_url":"u-042","body":"notes 042"},
        {"tag_name":"v0.4.10","html_url":"u-0410","body":"notes 0410"},
        {"tag_name":"v0.9.0","draft":true,"html_url":"u-draft"}
    ])";
    const auto r = UpdateChecker::evaluate("0.3.0", feed);
    REQUIRE(r.available);
    CHECK(r.version == "0.4.10");   // highest non-draft, non-prerelease (10 > 2)
    CHECK(r.url == "u-0410");
    CHECK(r.notes == "notes 0410");
}

TEST_CASE("UpdateChecker: malformed feed or bad current version -> no update")
{
    CHECK_FALSE(UpdateChecker::evaluate("0.3.0", "not json").available);
    CHECK_FALSE(UpdateChecker::evaluate("0.3.0", "[]").available);
    CHECK_FALSE(UpdateChecker::evaluate("bogus", R"({"version":"9.9.9"})").available);
}
