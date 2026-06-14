#include <catch2/catch.hpp>

#include "TestHelpers.h"
#include "chain/SmoothedValue.h"
#include "eq/Biquad.h"

using namespace veyra::dsp;

TEST_CASE("SmoothedValue glides to its target without overshoot")
{
    SmoothedValue s;
    s.prepare(48000.0, 5.0);
    s.reset(0.0f);
    s.setTarget(1.0f);

    REQUIRE(s.isSmoothing());

    float prev = 0.0f;
    for (int i = 0; i < 4800; ++i) // 100 ms >> 5 ms time constant
    {
        const float v = s.next();
        REQUIRE(v >= prev - 1e-6f); // monotonic up
        REQUIRE(v <= 1.0f + 1e-5f); // never overshoots
        prev = v;
    }
    REQUIRE(s.current() == Approx(1.0f).margin(1e-3));
    REQUIRE_FALSE(s.isSmoothing());
}

TEST_CASE("SmoothedValue::skip matches repeated next()")
{
    SmoothedValue a, b;
    a.prepare(48000.0, 5.0); a.reset(0.0f); a.setTarget(1.0f);
    b.prepare(48000.0, 5.0); b.reset(0.0f); b.setTarget(1.0f);

    for (int i = 0; i < 200; ++i) a.next();
    b.skip(200);

    REQUIRE(b.current() == Approx(a.current()).margin(1e-4));
}

TEST_CASE("Biquad: 0 dB peaking is transparent")
{
    Biquad f;
    f.setCoeffs(makePeaking(48000.0, 1000.0, 1.0, 0.0));

    auto in = th::sine(1000.0, 48000.0, 8192, 0.5f);
    std::vector<float> out(in.size());
    for (size_t i = 0; i < in.size(); ++i) out[i] = f.process(in[i]);

    REQUIRE(th::rmsTail(out) == Approx(th::rmsTail(in)).epsilon(0.02));
}

TEST_CASE("Biquad: low-pass attenuates high frequencies")
{
    Biquad f;
    f.setCoeffs(makeLowPass(48000.0, 1000.0, 0.707));

    auto in = th::sine(15000.0, 48000.0, 8192, 0.5f);
    std::vector<float> out(in.size());
    for (size_t i = 0; i < in.size(); ++i) out[i] = f.process(in[i]);

    // 15 kHz is nearly 4 octaves above a 1 kHz cutoff: heavily attenuated.
    REQUIRE(th::rmsTail(out) < 0.2 * th::rmsTail(in));
}

TEST_CASE("Biquad: high shelf boosts highs")
{
    Biquad f;
    f.setCoeffs(makeHighShelf(48000.0, 8000.0, 0.707, 12.0));

    auto in = th::sine(14000.0, 48000.0, 8192, 0.2f);
    std::vector<float> out(in.size());
    for (size_t i = 0; i < in.size(); ++i) out[i] = f.process(in[i]);

    REQUIRE(th::rmsTail(out) > 1.5 * th::rmsTail(in));
}
