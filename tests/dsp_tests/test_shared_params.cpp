#include <catch2/catch.hpp>

#include "veyra/ipc/SharedParameters.h"

using namespace veyra::ipc;

TEST_CASE("SharedParameters: publish/read round-trips the payload")
{
    VeyraSharedParameters shm{};
    shm.generation.store(0);

    VeyraParamsPayload in;
    in.bypass = 1;
    in.volumeGain = 2.5f;
    in.eqBandsDb[3] = 6.0f;
    in.stereoWidth = 1.75f;
    publishParameters(&shm, in);

    VeyraParamsPayload out;
    REQUIRE(readParameters(&shm, out));
    REQUIRE(out.bypass == 1u);
    REQUIRE(out.volumeGain == Approx(2.5f));
    REQUIRE(out.eqBandsDb[3] == Approx(6.0f));
    REQUIRE(out.stereoWidth == Approx(1.75f));
}

TEST_CASE("SharedParameters: reader backs off while a write is in progress")
{
    VeyraSharedParameters shm{};
    shm.generation.store(1); // odd == writer mid-update

    VeyraParamsPayload out;
    REQUIRE_FALSE(readParameters(&shm, out, 4));

    // Once the writer completes (even generation), reads succeed again.
    shm.generation.store(2);
    REQUIRE(readParameters(&shm, out, 4));
}
