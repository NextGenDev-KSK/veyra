#include <catch2/catch.hpp>

#include <cmath>
#include <vector>

#include "routing/OutputRouter.h"

using namespace veyra::dsp;

namespace {
constexpr double kFs = 48000.0;
int ms2s(float ms) { return (int) std::lround(ms * 0.001 * kFs); }
} // namespace

TEST_CASE("OutputRouter: compensation aligns routes to the slowest endpoint")
{
    OutputRouter rt;
    rt.prepare(kFs, 250.0f);
    rt.configure({
        {0.0f, 5.0f, true},   // fast endpoint -> needs the most delay
        {0.0f, 20.0f, true},  // slowest -> zero delay
        {0.0f, 12.0f, true},
    });

    REQUIRE(rt.routeCount() == 3);
    CHECK(rt.compensationSamples(0) == ms2s(15.0f)); // 20 - 5
    CHECK(rt.compensationSamples(1) == 0);           // 20 - 20
    CHECK(rt.compensationSamples(2) == ms2s(8.0f));  // 20 - 12
}

TEST_CASE("OutputRouter: per-output gain trim and disabled routes")
{
    OutputRouter rt;
    rt.prepare(kFs, 250.0f);
    rt.configure({
        {-6.0206f, 0.0f, true}, // ~ x0.5
        {0.0f, 0.0f, false},    // disabled -> silent
    });

    const int n = 16;
    std::vector<float> inL(n, 1.0f), inR(n, 1.0f), outL(n), outR(n);

    rt.renderRoute(0, inL.data(), inR.data(), outL.data(), outR.data(), n);
    CHECK(outL[0] == Approx(0.5f).margin(0.005f)); // gain applied, zero delay

    rt.renderRoute(1, inL.data(), inR.data(), outL.data(), outR.data(), n);
    CHECK_FALSE(rt.routeEnabled(1));
    CHECK(outL[0] == Approx(0.0f));                // disabled route is silent
}

TEST_CASE("OutputRouter: a single route needs no compensation")
{
    OutputRouter rt;
    rt.prepare(kFs);
    rt.configure({{0.0f, 30.0f, true}});
    CHECK(rt.compensationSamples(0) == 0);
}
