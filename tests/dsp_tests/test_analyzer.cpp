#include <catch2/catch.hpp>

#include <vector>

#include "TestHelpers.h"
#include "analyzer/Analyzer.h"
#include "analyzer/Fft.h"
#include "analyzer/SpscRingBuffer.h"

using namespace veyra::dsp;

TEST_CASE("Fft: a pure tone peaks in the expected bin")
{
    Fft fft;
    fft.prepare(10); // 1024-point

    const double fs = 48000.0;
    const int bin = 64;
    const double freq = bin * fs / 1024.0; // 3000 Hz

    auto in = th::sine(freq, fs, 1024, 1.0f);
    std::vector<float> mags(fft.numBins());
    fft.forwardMagnitude(in.data(), mags.data());

    size_t argmax = 0;
    for (size_t k = 1; k < mags.size(); ++k)
        if (mags[k] > mags[argmax]) argmax = k;

    REQUIRE(argmax >= static_cast<size_t>(bin - 1));
    REQUIRE(argmax <= static_cast<size_t>(bin + 1));
}

TEST_CASE("SpscRingBuffer: FIFO order and full/empty behaviour")
{
    SpscRingBuffer<int, 4> ring; // usable capacity 3

    REQUIRE(ring.empty());
    REQUIRE(ring.push(10));
    REQUIRE(ring.push(20));
    REQUIRE(ring.push(30));
    REQUIRE_FALSE(ring.push(40)); // full

    int v = 0;
    REQUIRE(ring.pop(v)); REQUIRE(v == 10);
    REQUIRE(ring.pop(v)); REQUIRE(v == 20);
    REQUIRE(ring.pop(v)); REQUIRE(v == 30);
    REQUIRE_FALSE(ring.pop(v)); // empty
}

TEST_CASE("Analyzer: reports RMS/peak and no clipping for a clean tone")
{
    Analyzer a;
    a.prepare(48000.0);

    auto in = th::sine(1000.0, 48000.0, kAnalyzerFftSize, 0.5f);
    a.processStereo(in.data(), in.data(), kAnalyzerFftSize);

    AnalyzerFrame f;
    REQUIRE(a.popFrame(f));
    REQUIRE(f.rmsL == Approx(0.5 / std::sqrt(2.0)).epsilon(0.05));
    REQUIRE(f.peakL == Approx(0.5f).margin(0.05));
    REQUIRE_FALSE(f.clipped);
}

TEST_CASE("Analyzer: flags clipping past full scale")
{
    Analyzer a;
    a.prepare(48000.0);

    auto in = th::sine(1000.0, 48000.0, kAnalyzerFftSize, 1.5f);
    a.processStereo(in.data(), in.data(), kAnalyzerFftSize);

    AnalyzerFrame f;
    REQUIRE(a.popFrame(f));
    REQUIRE(f.clipped);
    REQUIRE(f.peakL > 1.0f);
}
