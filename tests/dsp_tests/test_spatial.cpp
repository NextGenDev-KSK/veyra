#include <catch2/catch.hpp>

#include <cmath>
#include <vector>

#include "spatial/Convolver.h"
#include "spatial/Crossfeed.h"
#include "spatial/Hrtf.h"

using namespace veyra::dsp;

namespace {
constexpr double kFs = 48000.0;

float energy(const std::vector<float>& v)
{
    double e = 0.0;
    for (float x : v) e += (double) x * x;
    return (float) e;
}
} // namespace

TEST_CASE("PartitionedConvolver: identity IR is a passthrough")
{
    PartitionedConvolver c;
    const float ir[] = {1.0f};
    c.prepare(8, ir, 1);

    std::vector<float> in = {0.1f, -0.2f, 0.3f, 0.4f, -0.5f, 0.6f, 0.7f, -0.8f};
    std::vector<float> out(8, 0.0f);
    c.process(in.data(), out.data(), 8);
    for (int i = 0; i < 8; ++i)
        CHECK(out[(size_t) i] == Approx(in[(size_t) i]).margin(1e-4));
}

TEST_CASE("PartitionedConvolver: output equals the impulse response")
{
    // Feed a single impulse and read back the IR across several blocks.
    const std::vector<float> ir = {0.5f, -0.25f, 0.75f, 0.1f, -0.4f, 0.2f, 0.05f, -0.1f,
                                   0.3f, 0.15f, -0.2f, 0.6f}; // 12 taps -> 2 partitions @ B=8
    PartitionedConvolver c;
    c.prepare(8, ir.data(), (int) ir.size());

    std::vector<float> collected;
    for (int blk = 0; blk < 4; ++blk)
    {
        std::vector<float> in(8, 0.0f);
        if (blk == 0) in[0] = 1.0f; // impulse at the very start
        std::vector<float> out(8, 0.0f);
        c.process(in.data(), out.data(), 8);
        collected.insert(collected.end(), out.begin(), out.end());
    }

    for (size_t i = 0; i < ir.size(); ++i)
        CHECK(collected[i] == Approx(ir[i]).margin(1e-3));
    for (size_t i = ir.size(); i < collected.size(); ++i)
        CHECK(collected[i] == Approx(0.0f).margin(1e-3));
}

TEST_CASE("Crossfeed: amount 0 is a bit-exact bypass")
{
    Crossfeed cf;
    cf.prepare(kFs);
    cf.setAmount(0.0f);

    std::vector<float> l = {1.0f, 0.5f, -0.5f, 0.25f};
    std::vector<float> r = {0.0f, -0.5f, 0.5f, -0.25f};
    const auto l0 = l, r0 = r;
    cf.processStereo(l.data(), r.data(), (int) l.size());
    CHECK(l == l0);
    CHECK(r == r0);
}

TEST_CASE("Crossfeed: bleeds a hard-panned channel into the other")
{
    Crossfeed cf;
    cf.prepare(kFs);
    cf.setAmount(0.7f);

    const int n = 256;
    std::vector<float> l(n, 1.0f), r(n, 0.0f); // hard left
    cf.processStereo(l.data(), r.data(), n);

    // The right channel should now carry some low-passed, delayed bleed.
    CHECK(energy(r) > 0.0f);
    CHECK(r[n - 1] > 0.01f);
}

TEST_CASE("Crossfeed: tonal compensation keeps centred content ~flat")
{
    constexpr double kPi = 3.14159265358979323846;
    auto monoRms = [](double hz)
    {
        Crossfeed cf;
        cf.prepare(kFs);
        cf.setAmount(0.8f);
        const int n = 8192;
        std::vector<float> l(n), r(n);
        for (int i = 0; i < n; ++i)
            l[(size_t) i] = r[(size_t) i] = (float) std::sin(2.0 * kPi * hz * i / kFs); // mono
        cf.processStereo(l.data(), r.data(), n);
        double acc = 0.0;
        for (int i = n / 2; i < n; ++i) acc += (double) l[(size_t) i] * l[(size_t) i];
        return std::sqrt(acc / (n / 2));
    };
    const double lo = monoRms(200.0);
    const double hi = monoRms(10000.0);
    // Without compensation the highs would drop to ~0.6x the lows; the high-shelf
    // restores them so centred material stays roughly flat across the band.
    CHECK(hi > lo * 0.85);
}

TEST_CASE("BinauralPanner: a right-side source favours the right ear")
{
    const int B = 64;
    auto earEnergies = [B](float azimuth, float& eL, float& eR)
    {
        BinauralPanner p;
        p.prepare(B, kFs, azimuth, 128);
        std::vector<float> outL, outR;
        for (int blk = 0; blk < 4; ++blk)
        {
            std::vector<float> mono(B, 0.0f);
            if (blk == 0) mono[0] = 1.0f;
            std::vector<float> l(B, 0.0f), r(B, 0.0f);
            p.process(mono.data(), l.data(), r.data(), B);
            outL.insert(outL.end(), l.begin(), l.end());
            outR.insert(outR.end(), r.begin(), r.end());
        }
        eL = energy(outL);
        eR = energy(outR);
    };

    float eL, eR;
    earEnergies(60.0f, eL, eR);   // source to the right
    CHECK(eR > eL);

    float cL, cR;
    earEnergies(0.0f, cL, cR);    // centred -> roughly symmetric
    CHECK(cL == Approx(cR).margin(1e-3));
}
