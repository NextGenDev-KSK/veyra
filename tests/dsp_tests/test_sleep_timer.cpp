#include <catch2/catch.hpp>

#include "loudness/SleepTimer.h"

using namespace veyra::dsp;

TEST_CASE("SleepTimer: idle until started")
{
    SleepTimer t;
    t.configure(1.0f, 20.0f); // 1 min, 20 s fade
    CHECK_FALSE(t.armed());
    CHECK(t.advance(10.0) == Approx(1.0f)); // not armed -> unity
}

TEST_CASE("SleepTimer: holds unity before the fade window")
{
    SleepTimer t;
    t.configure(1.0f, 20.0f); // fade starts at 40 s
    t.start();
    CHECK(t.advance(30.0) == Approx(1.0f));
    CHECK(t.armed());
}

TEST_CASE("SleepTimer: monotonic fade to silence then finishes")
{
    SleepTimer t;
    t.configure(1.0f, 20.0f); // total 60 s, fade 40..60 s
    t.start();

    t.advance(40.0);                 // at the fade boundary
    const float g0 = t.gain();
    CHECK(g0 == Approx(1.0f).margin(0.02f));

    const float g1 = t.advance(10.0); // 50 s, half way through the fade
    CHECK(g1 < g0);
    CHECK(g1 > 0.0f);

    const float g2 = t.advance(5.0);  // 55 s
    CHECK(g2 < g1);

    const float g3 = t.advance(10.0); // 65 s -> past the end
    CHECK(g3 == Approx(0.0f));
    CHECK(t.finished());
    CHECK_FALSE(t.armed());

    // Stays silent until stopped.
    CHECK(t.advance(5.0) == Approx(0.0f));
}

TEST_CASE("SleepTimer: stop restores unity")
{
    SleepTimer t;
    t.configure(1.0f, 20.0f);
    t.start();
    t.advance(50.0);
    t.stop();
    CHECK_FALSE(t.armed());
    CHECK(t.gain() == Approx(1.0f));
    CHECK(t.advance(100.0) == Approx(1.0f));
}
