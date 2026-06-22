#include <catch2/catch.hpp>

#include <vector>

#include "spatial/RoomSimulator.h"

using namespace veyra::dsp;

namespace {
constexpr double kFs = 48000.0;

double energyRange(const std::vector<float>& v, int from, int to)
{
    double e = 0.0;
    for (int i = from; i < to && i < (int) v.size(); ++i) e += (double) v[(size_t) i] * v[(size_t) i];
    return e;
}
} // namespace

TEST_CASE("RoomSimulator adds early reflections after the direct sound")
{
    RoomSimulator room;
    room.prepare(kFs);
    room.setAmount(0.9f);

    const int n = 4096;
    std::vector<float> l(n, 0.0f), r(n, 0.0f);
    l[0] = 1.0f; // left-only click

    room.processStereo(l.data(), r.data(), n);

    // Reflections (7..41 ms ~ 336..1968 samples) arrive at both ears after t=0.
    CHECK(energyRange(r, 100, 2200) > 0.0);             // contralateral reflections
    CHECK(energyRange(l, 100, 2200) > 0.0);             // ipsilateral reflections too
    CHECK(l[0] == Approx(1.0f).margin(1e-6f));          // direct sound preserved
}

TEST_CASE("RoomSimulator at zero amount is an exact bypass")
{
    RoomSimulator room;
    room.prepare(kFs);
    room.setAmount(0.0f);
    std::vector<float> l = {0.5f, -0.3f, 0.2f, 0.9f}, r = {0.1f, 0.4f, -0.6f, 0.3f};
    const auto l0 = l, r0 = r;
    room.processStereo(l.data(), r.data(), (int) l.size());
    CHECK(l == l0);
    CHECK(r == r0);
}
