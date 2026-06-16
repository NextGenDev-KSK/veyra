#include <catch2/catch.hpp>

#include <cmath>
#include <vector>

#include "tracker/DirectionEstimator.h"
#include "tracker/FeatureExtractor.h"
#include "tracker/SoundClassifier.h"
#include "tracker/SoundTracker.h"

using namespace veyra::dsp;

namespace {
constexpr double kFs = 48000.0;
constexpr double kPi = 3.14159265358979323846;
} // namespace

TEST_CASE("DirectionEstimator: maps level difference to azimuth")
{
    CHECK(DirectionEstimator::fromStereoRms(1.0f, 0.0f).azimuthDeg == Approx(-90.0f));
    CHECK(DirectionEstimator::fromStereoRms(0.0f, 1.0f).azimuthDeg == Approx(90.0f));
    CHECK(DirectionEstimator::fromStereoRms(1.0f, 1.0f).azimuthDeg == Approx(0.0f).margin(1e-4));
    CHECK(DirectionEstimator::fromStereoRms(0.0f, 0.0f).azimuthDeg == Approx(0.0f).margin(1e-4));
}

TEST_CASE("FeatureExtractor: separates low and high frequency energy")
{
    const int n = 1024;
    auto tone = [n](double freq)
    {
        std::vector<float> v((size_t) n);
        for (int i = 0; i < n; ++i)
            v[(size_t) i] = 0.5f * (float) std::sin(2.0 * kPi * freq * i / kFs);
        return v;
    };

    FeatureExtractor lowEx;
    lowEx.prepare(kFs);
    auto lowF = lowEx.process(tone(100.0).data(), n);
    CHECK(lowF.lowEnergy > lowF.highEnergy);

    FeatureExtractor highEx;
    highEx.prepare(kFs);
    auto highF = highEx.process(tone(6000.0).data(), n);
    CHECK(highF.highEnergy > highF.lowEnergy);
}

TEST_CASE("SoundClassifier: recognises coarse classes from features")
{
    SoundClassifier c; // default sensitivity

    AudioFeatures silence; silence.rms = 0.001f;
    CHECK(c.classify(silence) == SoundClass::None);

    AudioFeatures gun;
    gun.rms = 0.5f; gun.flux = 0.3f; gun.lowEnergy = 0.3f; gun.midEnergy = 0.3f; gun.highEnergy = 0.4f;
    CHECK(c.classify(gun) == SoundClass::Gunshot);

    AudioFeatures foot;
    foot.rms = 0.4f; foot.flux = 0.1f; foot.lowEnergy = 0.7f; foot.midEnergy = 0.2f; foot.highEnergy = 0.1f;
    CHECK(c.classify(foot) == SoundClass::Footstep);

    AudioFeatures voice;
    voice.rms = 0.2f; voice.flux = 0.01f; voice.lowEnergy = 0.2f; voice.midEnergy = 0.7f; voice.highEnergy = 0.1f;
    CHECK(c.classify(voice) == SoundClass::Voice);
}

TEST_CASE("SoundTracker: detects a right-side low-frequency burst")
{
    SoundTracker t;
    t.prepare(kFs);
    t.setSensitivity(0.8f);

    const int frame = (int) (kFs * 0.010);

    // A few frames of silence, then a loud right-channel 120 Hz burst (onset).
    auto pushSilence = [&](int frames)
    {
        std::vector<float> z((size_t) (frame * frames), 0.0f);
        t.processStereo(z.data(), z.data(), frame * frames);
    };
    pushSilence(3);

    const int n = frame * 4;
    std::vector<float> l((size_t) n, 0.0f), r((size_t) n);
    for (int i = 0; i < n; ++i)
        r[(size_t) i] = 0.6f * (float) std::sin(2.0 * kPi * 120.0 * i / kFs);
    t.processStereo(l.data(), r.data(), n);

    bool gotRightSide = false;
    TrackerEvent ev;
    while (t.popEvent(ev))
        if (ev.azimuthDeg > 30.0f) // clearly to the right
            gotRightSide = true;
    CHECK(gotRightSide);
}
