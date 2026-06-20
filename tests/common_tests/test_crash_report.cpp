#include <catch2/catch.hpp>

#include <filesystem>
#include <system_error>

#include "veyra/CrashReport.h"

using namespace veyra;

TEST_CASE("CrashReport: JSON round-trips all fields")
{
    CrashReport r;
    r.version = "0.3.0";
    r.gitCommit = "abc1234";
    r.timestamp = "2026-06-19T12:34:56Z";
    r.process = "veyra-service";
    r.exceptionCode = 0xC0000005u; // access violation
    r.address = 0x7ff6abcd1234ull;
    r.module = "veyra-apo.dll";
    r.message = "Access violation reading 0x0";

    auto rt = CrashReport::fromJson(r.toJson());
    REQUIRE(rt.has_value());
    CHECK(rt->version == "0.3.0");
    CHECK(rt->gitCommit == "abc1234");
    CHECK(rt->process == "veyra-service");
    CHECK(rt->exceptionCode == 0xC0000005u);
    CHECK(rt->address == 0x7ff6abcd1234ull);
    CHECK(rt->module == "veyra-apo.dll");
    CHECK(rt->message == "Access violation reading 0x0");
}

TEST_CASE("CrashReport: write then read back the latest")
{
    std::error_code ec;
    const auto dir = std::filesystem::temp_directory_path(ec)
                     / ("veyra-crash-test-" + std::to_string((unsigned long long) ::time(nullptr)));
    std::filesystem::remove_all(dir, ec);

    CHECK_FALSE(latestCrashReport(dir).has_value()); // empty/missing dir

    CrashReport r;
    r.version = "0.3.0";
    r.timestamp = "2026-06-19T00:00:01Z";
    r.process = "veyra";
    r.message = "boom";
    const auto path = writeCrashReport(r, dir);
    CHECK_FALSE(path.empty());

    auto latest = latestCrashReport(dir);
    REQUIRE(latest.has_value());
    CHECK(latest->message == "boom");
    CHECK(latest->process == "veyra");

    std::filesystem::remove_all(dir, ec);
}

TEST_CASE("CrashReport: malformed JSON returns nullopt")
{
    CHECK_FALSE(CrashReport::fromJson("not json").has_value());
    CHECK_FALSE(CrashReport::fromJson("[1,2,3]").has_value());
}
