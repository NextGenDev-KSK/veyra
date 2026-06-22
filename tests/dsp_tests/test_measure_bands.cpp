#include <catch2/catch.hpp>

#include <cmath>
#include <vector>

#include "analyzer/MeasureBands.h"

using namespace veyra::dsp;

namespace {
constexpr double kFs = 48000.0;
constexpr double kPi = 3.14159265358979323846;
} // namespace

TEST_CASE("MeasureBands: a tone reads loudest in its own band")
{
    MeasureBands m;
    m.prepare(kFs);

    // 1 kHz sits at band 5 (31.25 * 2^5 = 1000).
    const int n = 48000; // ~1 s to settle the integrator
    std::vector<float> x((size_t) n);
    for (int i = 0; i < n; ++i) x[(size_t) i] = 0.5f * (float) std::sin(2.0 * kPi * 1000.0 * i / kFs);
    m.process(x.data(), n);

    CHECK(m.centreHz(5) == Approx(1000.0f));
    const float peak = m.levelLinear(5);
    for (int b = 0; b < MeasureBands::kBands; ++b)
        if (b != 5)
            CHECK(m.levelLinear(b) < peak * 0.5f); // neighbouring bands clearly lower
}

TEST_CASE("MeasureBands: silence reads the floor")
{
    MeasureBands m;
    m.prepare(kFs);
    std::vector<float> z(4096, 0.0f);
    m.process(z.data(), (int) z.size());
    for (int b = 0; b < MeasureBands::kBands; ++b)
        CHECK(m.levelDb(b) < -100.0f);
}
