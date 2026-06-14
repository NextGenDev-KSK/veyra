#include <catch2/catch.hpp>

#include <cmath>
#include <vector>

#include "TestHelpers.h"
#include "chain/DspChain.h"
#include "chain/DspParameters.h"

using namespace veyra::dsp;

TEST_CASE("DspChain: bypass passes audio through unchanged")
{
    DspChain chain;
    chain.prepare(48000.0);
    DspParameters p;
    p.bypass = true;
    chain.setParameters(p);

    auto in = th::sine(1000.0, 48000.0, 4096, 0.3f);
    std::vector<float> L = in, R = in;
    chain.processStereo(L.data(), R.data(), static_cast<int>(L.size()));

    for (size_t i = 0; i < in.size(); ++i)
        REQUIRE(L[i] == Approx(in[i]));
}

TEST_CASE("DspChain: zero volume gain silences output")
{
    DspChain chain;
    chain.prepare(48000.0);
    DspParameters p;
    p.volumeGain = 0.0f;
    chain.setParameters(p);

    auto in = th::sine(1000.0, 48000.0, 24000, 0.5f);
    std::vector<float> L = in, R = in;
    chain.processStereo(L.data(), R.data(), static_cast<int>(L.size()));

    REQUIRE(th::rmsTail(L, 0.25) < 1e-3);
}

TEST_CASE("DspChain: EQ band boost is audible end-to-end")
{
    DspChain chain;
    chain.prepare(48000.0);
    DspParameters p;
    p.eqBandsDb[5] = 12.0f; // 1 kHz
    chain.setParameters(p);

    auto in = th::sine(1000.0, 48000.0, 24000, 0.1f);
    std::vector<float> L = in, R = in;
    chain.processStereo(L.data(), R.data(), static_cast<int>(L.size()));

    REQUIRE(th::rmsTail(L) > 1.5 * th::rmsTail(in));
    // No NaNs / infs slipped through.
    for (float x : L) REQUIRE(std::isfinite(x));
}

TEST_CASE("DspChain: limiter ceiling holds even with large make-up gain")
{
    DspChain chain;
    chain.prepare(48000.0);
    DspParameters p;
    p.volumeGain = 3.0f;            // push well past full scale
    p.limiterCeilingDb = -0.3f;
    chain.setParameters(p);

    auto in = th::sine(220.0, 48000.0, 24000, 0.5f);
    std::vector<float> L = in, R = in;
    chain.processStereo(L.data(), R.data(), static_cast<int>(L.size()));

    const float ceiling = std::pow(10.0f, -0.3f / 20.0f);
    // Check the settled tail (after volume ramp + lookahead).
    const int n = static_cast<int>(L.size());
    for (int i = n / 2; i < n; ++i)
        REQUIRE(std::fabs(L[static_cast<size_t>(i)]) <= ceiling * 1.001f);
}
