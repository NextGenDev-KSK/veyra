#include <catch2/catch.hpp>

#include <chrono>
#include <cmath>
#include <cstdio>
#include <filesystem>
#include <vector>

#include "spatial/HrtfDatabase.h"
#include "spatial/WavReader.h"

using namespace veyra::dsp;
namespace fs = std::filesystem;

#ifndef VEYRA_KEMAR_DIR
#define VEYRA_KEMAR_DIR ""
#endif

namespace {
const fs::path kDir{VEYRA_KEMAR_DIR}; // <repo>/third_party/hrtf/mit_kemar/diffuse

double energy(const std::vector<float>& v)
{
    double e = 0.0;
    for (float s : v) e += (double) s * s;
    return e;
}
} // namespace

TEST_CASE("KEMAR WAV reader parses a measured HRIR file")
{
    REQUIRE(fs::is_directory(kDir));
    WavData w;
    REQUIRE(readWav(kDir / "elev0" / "H0e000a.wav", w));
    CHECK(w.channels == 2);          // compact set: L-ear + R-ear
    CHECK(w.sampleRate == 44100);
    CHECK(w.ch[0].size() == 128);    // 128-tap compact IR
}

TEST_CASE("HrtfDatabase indexes the diffuse set")
{
    HrtfDatabase db;
    REQUIRE(db.load(kDir));
    CHECK(db.loaded());
    CHECK(db.measurementCount() > 100); // hundreds of (elev,az) measurements
}

TEST_CASE("HrtfDatabase: front is ~symmetric, sides favour the near ear")
{
    HrtfDatabase db;
    REQUIRE(db.load(kDir));
    std::vector<float> l, r;

    REQUIRE(db.getHrir(0.0f, 0.0f, 44100.0, l, r));
    CHECK(energy(l) == Approx(energy(r)).epsilon(0.05)); // centred source

    REQUIRE(db.getHrir(90.0f, 0.0f, 44100.0, l, r));  // hard right
    CHECK(energy(r) > energy(l) * 1.3);

    REQUIRE(db.getHrir(-90.0f, 0.0f, 44100.0, l, r)); // hard left (mirror)
    CHECK(energy(l) > energy(r) * 1.3);
}

TEST_CASE("HrtfDatabase: resamples to the engine rate")
{
    HrtfDatabase db;
    REQUIRE(db.load(kDir));
    std::vector<float> l, r;
    REQUIRE(db.getHrir(30.0f, 0.0f, 48000.0, l, r));
    // 128 taps @44.1k -> ~139 @48k
    CHECK(l.size() >= 135);
    CHECK(l.size() <= 143);
    CHECK(l.size() == r.size());
}

TEST_CASE("HrtfDatabase: interpolates between measured azimuths")
{
    HrtfDatabase db;
    REQUIRE(db.load(kDir));
    std::vector<float> l, r;
    REQUIRE(db.getHrir(22.0f, 0.0f, 44100.0, l, r)); // between 20 and 25
    CHECK(!l.empty());
    for (float s : l) CHECK(std::isfinite(s));
}

TEST_CASE("HrtfDatabase: load + query are fast (benchmark)")
{
    const auto t0 = std::chrono::steady_clock::now();
    HrtfDatabase db;
    REQUIRE(db.load(kDir));
    const auto t1 = std::chrono::steady_clock::now();

    std::vector<float> l, r;
    const int q = 2000;
    for (int i = 0; i < q; ++i)
        db.getHrir((float) (i % 181) - 90.0f, 0.0f, 48000.0, l, r);
    const auto t2 = std::chrono::steady_clock::now();

    const double loadMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    const double qMs    = std::chrono::duration<double, std::milli>(t2 - t1).count();
    std::printf("[hrtf] index %d measurements in %.0f ms; %d queries in %.0f ms\n",
                db.measurementCount(), loadMs, q, qMs);
    CHECK(loadMs < 3000.0);
    CHECK(qMs < 500.0); // cached fetch keeps repeated queries cheap
}
