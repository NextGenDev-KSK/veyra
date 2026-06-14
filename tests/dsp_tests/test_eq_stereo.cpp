#include <catch2/catch.hpp>

#include "TestHelpers.h"
#include "enhancers/StereoProcessor.h"
#include "enhancers/ToneControls.h"
#include "eq/GraphicEq.h"

using namespace veyra::dsp;

TEST_CASE("GraphicEq: boosting the 1 kHz band lifts a 1 kHz tone")
{
    GraphicEq eq;
    eq.prepare(48000.0);
    eq.setBandGainDb(5, 12.0f); // band index 5 == 1 kHz centre

    auto inL = th::sine(1000.0, 48000.0, 24000, 0.1f);
    auto inR = inL;
    auto L = inL, R = inR;
    eq.processStereo(L.data(), R.data(), static_cast<int>(L.size()));

    REQUIRE(th::rmsTail(L) > 1.8 * th::rmsTail(inL));
}

TEST_CASE("GraphicEq: cutting a band attenuates that tone")
{
    GraphicEq eq;
    eq.prepare(48000.0);
    eq.setBandGainDb(5, -12.0f);

    auto inL = th::sine(1000.0, 48000.0, 24000, 0.3f);
    auto L = inL, R = inL;
    eq.processStereo(L.data(), R.data(), static_cast<int>(L.size()));

    REQUIRE(th::rmsTail(L) < 0.6 * th::rmsTail(inL));
}

TEST_CASE("ToneControls: bass shelf lifts low frequencies")
{
    ToneControls tone;
    tone.prepare(48000.0);
    tone.setBassDb(12.0f);

    auto inL = th::sine(50.0, 48000.0, 24000, 0.2f);
    auto L = inL, R = inL;
    tone.processStereo(L.data(), R.data(), static_cast<int>(L.size()));

    REQUIRE(th::rmsTail(L) > 1.5 * th::rmsTail(inL));
}

TEST_CASE("StereoProcessor: width 0 collapses a side-only signal")
{
    StereoProcessor sp;
    sp.prepare(48000.0);
    sp.setWidth(0.0f);

    // Pure side signal: L = +a, R = -a.
    auto a = th::sine(1000.0, 48000.0, 8192, 0.5f);
    std::vector<float> L = a, R(a.size());
    for (size_t i = 0; i < a.size(); ++i) R[i] = -a[i];

    sp.applyWidth(L.data(), R.data(), static_cast<int>(L.size()));

    // After settling, side is removed -> near silence.
    REQUIRE(th::rmsTail(L, 0.25) < 0.01);
    REQUIRE(th::rmsTail(R, 0.25) < 0.01);
}

TEST_CASE("StereoProcessor: hard-right balance mutes the left channel")
{
    StereoProcessor sp;
    sp.prepare(48000.0);
    sp.setBalance(1.0f);

    auto tone = th::sine(1000.0, 48000.0, 12000, 0.5f);
    std::vector<float> L = tone, R = tone;
    sp.applyMonoBalance(L.data(), R.data(), static_cast<int>(L.size()));

    REQUIRE(th::rmsTail(L, 0.25) < 0.02);
    REQUIRE(th::rmsTail(R, 0.25) == Approx(th::rmsTail(tone)).epsilon(0.05));
}

TEST_CASE("StereoProcessor: mono makes both channels equal")
{
    StereoProcessor sp;
    sp.prepare(48000.0);
    sp.setMono(true);

    auto tone = th::sine(1000.0, 48000.0, 12000, 0.5f);
    std::vector<float> L = tone, R(tone.size(), 0.0f); // signal only on the left
    sp.applyMonoBalance(L.data(), R.data(), static_cast<int>(L.size()));

    const int n = static_cast<int>(L.size());
    for (int i = n * 3 / 4; i < n; ++i)
        REQUIRE(L[static_cast<size_t>(i)] == Approx(R[static_cast<size_t>(i)]).margin(1e-4));
}
