#include <catch2/catch.hpp>

#include "veyra/ipc/TrackerData.h"

using namespace veyra::ipc;

namespace {
TrackerEventRecord mk(int type, float az)
{
    TrackerEventRecord r;
    r.type = type;
    r.azimuthDeg = az;
    return r;
}
} // namespace

TEST_CASE("TrackerData: consumer drains only new events")
{
    VeyraTrackerData shm{};
    uint64_t seen = 0;
    TrackerEventRecord out[64];

    CHECK(readTrackerEvents(&shm, seen, out, 64) == 0);

    writeTrackerEvent(&shm, mk(1, -45.0f));
    writeTrackerEvent(&shm, mk(2, 30.0f));

    const int n = readTrackerEvents(&shm, seen, out, 64);
    REQUIRE(n == 2);
    CHECK(out[0].type == 1);
    CHECK(out[0].azimuthDeg == Approx(-45.0f));
    CHECK(out[1].type == 2);

    // Nothing new yet.
    CHECK(readTrackerEvents(&shm, seen, out, 64) == 0);

    writeTrackerEvent(&shm, mk(3, 0.0f));
    REQUIRE(readTrackerEvents(&shm, seen, out, 64) == 1);
    CHECK(out[0].type == 3);
}

TEST_CASE("TrackerData: a lagging consumer resyncs to available events")
{
    VeyraTrackerData shm{};
    uint64_t seen = 0;

    // Write more than the ring capacity without reading.
    const int total = VeyraTrackerData::kCapacity * 3;
    for (int i = 0; i < total; ++i)
        writeTrackerEvent(&shm, mk(i % 4, (float) i));

    TrackerEventRecord out[VeyraTrackerData::kCapacity + 8];
    const int n = readTrackerEvents(&shm, seen, out, VeyraTrackerData::kCapacity + 8);

    // At most the ring capacity is recoverable, and we caught up to the writer.
    CHECK(n == VeyraTrackerData::kCapacity);
    CHECK(seen == (uint64_t) total);
    // The newest event must be present and correct.
    CHECK(out[n - 1].azimuthDeg == Approx((float) (total - 1)));
}
