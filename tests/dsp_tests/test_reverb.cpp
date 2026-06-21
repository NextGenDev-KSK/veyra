#include <catch2/catch.hpp>

#include <cmath>
#include <vector>

#include "enhancers/Reverb.h"

using namespace veyra::dsp;

TEST_CASE("Reverb: amount 0 is passthrough")
{
    Reverb r;
    r.prepare(48000.0);
    r.setAmount(0.0f);
    std::vector<float> l{1, 0, 0, 0, 0}, rr{1, 0, 0, 0, 0};
    r.processStereo(l.data(), rr.data(), 5);
    CHECK(l[0] == Approx(1.0f));
    CHECK(l[4] == Approx(0.0f));
}

TEST_CASE("Reverb: a wet impulse leaves a decaying tail")
{
    Reverb r;
    r.prepare(48000.0);
    r.setAmount(0.8f);

    const int n = 24000;
    std::vector<float> l(n, 0.0f), rr(n, 0.0f);
    l[0] = 1.0f; rr[0] = 1.0f;
    r.processStereo(l.data(), rr.data(), n);

    double tail = 0.0;
    bool finite = true;
    for (int i = 2000; i < n; ++i)
    {
        if (!std::isfinite(l[i]) || !std::isfinite(rr[i])) finite = false;
        tail += std::abs(l[i]) + std::abs(rr[i]);
    }
    CHECK(finite);
    CHECK(tail > 0.0); // there is reverberant energy after the dry impulse
}

TEST_CASE("Reverb: stays finite and bounded under sustained input")
{
    Reverb r;
    r.prepare(48000.0);
    r.setAmount(1.0f);

    const int n = 48000;
    std::vector<float> l(n, 0.5f), rr(n, 0.5f);
    r.processStereo(l.data(), rr.data(), n);

    float peak = 0.0f;
    for (int i = 0; i < n; ++i)
        peak = std::max(peak, std::max(std::abs(l[i]), std::abs(rr[i])));
    CHECK(std::isfinite(peak));
    CHECK(peak < 4.0f);
}
