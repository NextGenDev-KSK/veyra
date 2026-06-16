#include <catch2/catch.hpp>

#include <cmath>
#include <vector>

#include "voice/NoiseSuppressor.h"
#include "voice/VoiceChain.h"

using namespace veyra::dsp;

namespace {
constexpr double kFs = 48000.0;
constexpr double kPi = 3.14159265358979323846;

std::vector<float> sine(double freq, float amp, int n)
{
    std::vector<float> v((size_t) n);
    for (int i = 0; i < n; ++i)
        v[(size_t) i] = amp * (float) std::sin(2.0 * kPi * freq * i / kFs);
    return v;
}

float rms(const std::vector<float>& v, int from, int to)
{
    double acc = 0.0;
    for (int i = from; i < to; ++i)
        acc += (double) v[(size_t) i] * v[(size_t) i];
    return (float) std::sqrt(acc / std::max(1, to - from));
}
} // namespace

TEST_CASE("NoiseSuppressor: amount 0 is a bit-exact passthrough")
{
    NoiseSuppressor ns;
    ns.prepare(kFs);
    ns.setAmount(0.0f);

    auto buf = sine(440.0, 0.2f, 256);
    const auto orig = buf;
    ns.processMono(buf.data(), (int) buf.size());
    CHECK(buf == orig);
}

TEST_CASE("NoiseSuppressor: gates quiet signal but passes loud signal")
{
    const int n = (int) (kFs * 0.5); // 0.5 s

    // Quiet tone (~ -46 dBFS) at full suppression -> heavily attenuated.
    {
        NoiseSuppressor ns;
        ns.prepare(kFs);
        ns.setAmount(1.0f);
        auto quiet = sine(300.0, 0.005f, n);
        const float inRms = rms(quiet, 0, n);
        ns.processMono(quiet.data(), n);
        const float outRms = rms(quiet, n / 2, n); // after the gate settles
        CHECK(outRms < inRms * 0.3f);
    }

    // Loud tone (~ -10 dBFS) at full suppression -> essentially untouched.
    {
        NoiseSuppressor ns;
        ns.prepare(kFs);
        ns.setAmount(1.0f);
        auto loud = sine(300.0, 0.3f, n);
        const float inRms = rms(loud, 0, n);
        ns.processMono(loud.data(), n);
        const float outRms = rms(loud, n / 2, n);
        CHECK(outRms > inRms * 0.8f);
    }
}

TEST_CASE("VoiceChain: disabled is a bit-exact passthrough")
{
    VoiceChain vc;
    vc.prepare(kFs);
    VoiceParams p;
    p.enabled = false;
    vc.setParams(p);

    auto buf = sine(1000.0, 0.2f, 512);
    const auto orig = buf;
    vc.processMono(buf.data(), (int) buf.size());
    CHECK(buf == orig);
}

TEST_CASE("VoiceChain: high-pass removes a DC offset")
{
    VoiceChain vc;
    vc.prepare(kFs);
    VoiceParams p;            // enabled, 80 Hz HPF
    p.noiseSuppression = 0.0f; // isolate the filter
    p.compressionAmount = 0.0f;
    p.deEssAmount = 0.0f;
    p.presenceDb = 0.0f;
    vc.setParams(p);

    std::vector<float> buf((size_t) (kFs * 0.5), 0.5f); // constant DC
    vc.processMono(buf.data(), (int) buf.size());

    double mean = 0.0;
    const int from = (int) buf.size() * 3 / 4;
    for (int i = from; i < (int) buf.size(); ++i)
        mean += buf[(size_t) i];
    mean /= (buf.size() - from);
    CHECK(std::fabs(mean) < 0.02);
}

TEST_CASE("VoiceChain: a normal-level voice tone passes through")
{
    VoiceChain vc;
    vc.prepare(kFs);
    vc.setParams(VoiceParams{}); // defaults

    const int n = (int) (kFs * 0.3);
    auto buf = sine(1000.0, 0.12f, n);
    const float inRms = rms(buf, 0, n);
    vc.processMono(buf.data(), n);
    const float outRms = rms(buf, n / 2, n);
    CHECK(outRms > inRms * 0.4f); // not gated away
}

TEST_CASE("VoiceChain: de-esser attenuates sibilance more than low frequencies")
{
    auto ratioForFreq = [](double freq)
    {
        VoiceChain vc;
        vc.prepare(kFs);
        VoiceParams p;
        p.noiseSuppression = 0.0f;
        p.compressionAmount = 0.0f;
        p.presenceDb = 0.0f;
        p.deEssAmount = 1.0f;
        vc.setParams(p);

        const int n = (int) (kFs * 0.3);
        auto buf = sine(freq, 0.3f, n);
        const float inRms = rms(buf, 0, n);
        vc.processMono(buf.data(), n);
        return rms(buf, n / 2, n) / inRms;
    };

    const float lowRatio  = ratioForFreq(500.0);  // below the de-ess band
    const float highRatio = ratioForFreq(7500.0); // sibilance band
    CHECK(highRatio < lowRatio * 0.9f);
}
