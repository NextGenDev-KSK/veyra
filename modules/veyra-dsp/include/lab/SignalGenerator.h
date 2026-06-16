#pragma once

// Sound Lab test-signal generator. Backs the lab's tools: sine tone, log
// frequency sweep, white/pink noise, silence, plus channel routing (both /
// left / right) and a right-channel polarity invert for the speaker placement
// and phase tests. Allocation-free and deterministic (seeded PRNG).

#include <algorithm>
#include <cmath>
#include <cstdint>

namespace veyra::dsp {

enum class TestSignal { Silence, SineTone, Sweep, WhiteNoise, PinkNoise };
enum class TestChannel { Both, Left, Right };

struct LabSignalParams {
    TestSignal  type = TestSignal::SineTone;
    TestChannel channel = TestChannel::Both;
    float frequency = 1000.0f;                 // SineTone, Hz
    float sweepStartHz = 20.0f;                // Sweep
    float sweepEndHz = 20000.0f;
    float sweepSeconds = 4.0f;
    float level = 0.5f;                        // amplitude 0..1
    bool  invertRight = false;                 // polarity / phase test
};

class SignalGenerator {
public:
    void prepare(double sampleRate) noexcept
    {
        sampleRate_ = sampleRate;
        reset();
    }

    void reset() noexcept
    {
        phase_ = 0.0;
        sweepT_ = 0.0;
        rng_ = 0x9E3779B9u;
        p0_ = p1_ = p2_ = 0.0f;
    }

    void setParams(const LabSignalParams& p) noexcept { params_ = p; }
    const LabSignalParams& params() const noexcept { return params_; }

    void renderStereo(float* left, float* right, int numSamples) noexcept
    {
        for (int i = 0; i < numSamples; ++i)
        {
            const float s = nextSample();
            const float l = (params_.channel == TestChannel::Right) ? 0.0f : s;
            float r = (params_.channel == TestChannel::Left) ? 0.0f : s;
            if (params_.invertRight)
                r = -r;
            left[i] = l;
            right[i] = r;
        }
    }

private:
    float nextSample() noexcept
    {
        const float lvl = params_.level;
        switch (params_.type)
        {
        case TestSignal::Silence:
            return 0.0f;

        case TestSignal::SineTone:
        {
            const float s = lvl * (float) std::sin(phase_);
            phase_ += 2.0 * kPi * params_.frequency / sampleRate_;
            if (phase_ > 2.0 * kPi) phase_ -= 2.0 * kPi;
            return s;
        }

        case TestSignal::Sweep:
        {
            // Exponential (log) sweep, looping over sweepSeconds.
            const double frac = sweepT_ / std::max(0.01f, params_.sweepSeconds);
            const double f = params_.sweepStartHz *
                             std::pow(params_.sweepEndHz / params_.sweepStartHz, frac);
            const float s = lvl * (float) std::sin(phase_);
            phase_ += 2.0 * kPi * f / sampleRate_;
            if (phase_ > 2.0 * kPi) phase_ -= 2.0 * kPi;
            sweepT_ += 1.0 / sampleRate_;
            if (sweepT_ >= params_.sweepSeconds) sweepT_ = 0.0;
            return s;
        }

        case TestSignal::WhiteNoise:
            return lvl * white();

        case TestSignal::PinkNoise:
        {
            // Paul Kellet's economy pink filter.
            const float w = white();
            p0_ = 0.99765f * p0_ + w * 0.0990460f;
            p1_ = 0.96300f * p1_ + w * 0.2965164f;
            p2_ = 0.57000f * p2_ + w * 1.0526913f;
            return lvl * 0.2f * (p0_ + p1_ + p2_ + w * 0.1848f);
        }
        }
        return 0.0f;
    }

    float white() noexcept
    {
        // xorshift32 -> [-1, 1)
        rng_ ^= rng_ << 13;
        rng_ ^= rng_ >> 17;
        rng_ ^= rng_ << 5;
        return (float) ((int32_t) rng_) / 2147483648.0f;
    }

    static constexpr double kPi = 3.14159265358979323846;

    double sampleRate_ = 48000.0;
    LabSignalParams params_;
    double phase_ = 0.0, sweepT_ = 0.0;
    uint32_t rng_ = 0x9E3779B9u;
    float p0_ = 0.0f, p1_ = 0.0f, p2_ = 0.0f;
};

} // namespace veyra::dsp
