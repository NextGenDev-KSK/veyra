#include <catch2/catch.hpp>

#include "veyra/ipc/AnalyzerData.h"

using namespace veyra::ipc;

TEST_CASE("AnalyzerData: publish/read round-trips the metering frame")
{
    VeyraAnalyzerData shm{};
    shm.generation.store(0);

    VeyraAnalyzerPayload in;
    in.vuL = 0.6f;
    in.vuR = 0.5f;
    in.peakL = 0.8f;
    in.peakR = 0.7f;
    in.clip = 1;
    for (int i = 0; i < kAnalyzerBars; ++i)
        in.bars[i] = (float) i / kAnalyzerBars;
    publishAnalyzer(&shm, in);

    VeyraAnalyzerPayload out;
    REQUIRE(readAnalyzer(&shm, out));
    CHECK(out.vuL == Approx(0.6f));
    CHECK(out.vuR == Approx(0.5f));
    CHECK(out.peakL == Approx(0.8f));
    CHECK(out.clip == 1u);
    CHECK(out.bars[0] == Approx(0.0f));
    CHECK(out.bars[kAnalyzerBars - 1] == Approx((float) (kAnalyzerBars - 1) / kAnalyzerBars));
}
