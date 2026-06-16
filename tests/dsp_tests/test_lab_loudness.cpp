#include <catch2/catch.hpp>

#include <cmath>
#include <vector>

#include "lab/SignalGenerator.h"
#include "loudness/NightMode.h"
#include "loudness/SleepFade.h"

using namespace veyra::dsp;

namespace {
constexpr double kFs = 48000.0;

float rms(const std::vector<float>& v)
{
    double e = 0.0;
    for (float x : v) e += (double) x * x;
    return (float) std::sqrt(e / std::max<size_t>(1, v.size()));
}
float peak(const std::vector<float>& v)
{
    float p = 0.0f;
    for (float x : v) p = std::max(p, std::fabs(x));
    return p;
}
} // namespace

TEST_CASE("SignalGenerator: sine level and channel routing")
{
    SignalGenerator g;
    g.prepare(kFs);
    LabSignalParams p;
    p.type = TestSignal::SineTone;
    p.frequency = 1000.0f;
    p.level = 0.5f;
    p.channel = TestChannel::Left;
    g.setParams(p);

    const int n = 4800;
    std::vector<float> l((size_t) n), r((size_t) n);
    g.renderStereo(l.data(), r.data(), n);

    CHECK(peak(l) == Approx(0.5f).margin(0.02f));
    CHECK(rms(l) == Approx(0.5f / std::sqrt(2.0f)).margin(0.02f));
    CHECK(peak(r) == Approx(0.0f).margin(1e-6f)); // routed to left only
}

TEST_CASE("SignalGenerator: right-channel polarity invert")
{
    SignalGenerator g;
    g.prepare(kFs);
    LabSignalParams p;
    p.type = TestSignal::SineTone;
    p.channel = TestChannel::Both;
    p.invertRight = true;
    g.setParams(p);

    const int n = 512;
    std::vector<float> l((size_t) n), r((size_t) n);
    g.renderStereo(l.data(), r.data(), n);
    for (int i = 0; i < n; ++i)
        CHECK(r[(size_t) i] == Approx(-l[(size_t) i]).margin(1e-6f));
}

TEST_CASE("SignalGenerator: silence and noise bounds")
{
    SignalGenerator g;
    g.prepare(kFs);

    LabSignalParams sil; sil.type = TestSignal::Silence;
    g.setParams(sil);
    std::vector<float> l(256), r(256);
    g.renderStereo(l.data(), r.data(), 256);
    CHECK(peak(l) == Approx(0.0f).margin(1e-9f));

    LabSignalParams wn; wn.type = TestSignal::WhiteNoise; wn.level = 0.5f;
    g.setParams(wn);
    g.renderStereo(l.data(), r.data(), 256);
    CHECK(rms(l) > 0.05f);   // actually producing noise
    CHECK(peak(l) <= 0.5f + 1e-4f);
}

TEST_CASE("NightMode: bypass at 0, compresses dynamics at 1")
{
    SECTION("amount 0 is passthrough")
    {
        NightMode nm; nm.prepare(kFs); nm.setAmount(0.0f);
        std::vector<float> l = {0.8f, -0.6f, 0.4f, -0.2f}, r = l;
        const auto l0 = l;
        nm.processStereo(l.data(), r.data(), (int) l.size());
        CHECK(l == l0);
    }

    auto outRms = [](float amp)
    {
        NightMode nm; nm.prepare(kFs); nm.setAmount(1.0f);
        const int n = (int) (kFs * 0.5);
        std::vector<float> l((size_t) n), r((size_t) n);
        for (int i = 0; i < n; ++i)
            l[(size_t) i] = r[(size_t) i] = amp * (float) std::sin(2.0 * 3.14159265 * 200.0 * i / kFs);
        const float inR = rms(l);
        nm.processStereo(l.data(), r.data(), n);
        return std::make_pair(inR, rms(l));
    };

    auto [loudIn, loudOut] = outRms(0.8f);
    CHECK(loudOut < loudIn);   // loud material pulled down

    auto [quietIn, quietOut] = outRms(0.03f);
    CHECK(quietOut > quietIn); // quiet material lifted by make-up
}

TEST_CASE("SleepFade: monotonic exponential fade to silence")
{
    SleepFade f;
    f.prepare(kFs);

    // Inactive => passthrough.
    std::vector<float> a = {1.0f, 1.0f}, b = {1.0f, 1.0f};
    f.processStereo(a.data(), b.data(), 2);
    CHECK(a[0] == Approx(1.0f));

    f.start(1.0f); // 1-second fade
    const int n = (int) (kFs * 1.2);
    std::vector<float> l((size_t) n, 1.0f), r((size_t) n, 1.0f);
    f.processStereo(l.data(), r.data(), n);

    CHECK(l[0] == Approx(1.0f).margin(0.01f)); // starts near unity
    for (int i = 1; i < n; ++i)
        CHECK(l[(size_t) i] <= l[(size_t) i - 1] + 1e-6f); // non-increasing
    CHECK(l[(size_t) (n - 1)] < 0.01f); // ends in silence
    CHECK(f.finished());
}
