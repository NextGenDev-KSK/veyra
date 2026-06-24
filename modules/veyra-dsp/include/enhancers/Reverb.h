#pragma once

// Freeverb-style stereo reverb: 8 comb filters (4 per channel) into 4 allpass
// (2 per channel), wet/dry mixed. Header-only, allocation-free (fixed buffers
// sized for up to ~96 kHz; higher rates clamp to a shorter tail), RT-safe.

#include <algorithm>
#include <array>
#include <cmath>

namespace veyra::dsp {

class Reverb {
public:
    void prepare(double sampleRate) noexcept
    {
        sampleRate_ = sampleRate > 0.0 ? sampleRate : 48000.0;
        const double scale = sampleRate_ / 44100.0;
        for (int i = 0; i < kCombs; ++i)
        {
            combL_[i].setSize(kCombTuning[i], scale);
            combR_[i].setSize(kCombTuning[i] + kStereoSpread, scale);
        }
        for (int i = 0; i < kAllpasses; ++i)
        {
            apL_[i].setSize(kAllpassTuning[i], scale);
            apR_[i].setSize(kAllpassTuning[i] + kStereoSpread, scale);
        }
        reset();
        setAmount(amount_);
    }

    void reset() noexcept
    {
        for (auto& c : combL_) c.reset();
        for (auto& c : combR_) c.reset();
        for (auto& a : apL_) a.reset();
        for (auto& a : apR_) a.reset();
    }

    // 0 = dry/off, 1 = max wet + longest tail.
    void setAmount(float amount) noexcept
    {
        amount_ = std::clamp(amount, 0.0f, 1.0f);
        wet_ = amount_ * 0.6f;                       // keep it tasteful at 100%
        const float room = 0.70f + 0.28f * amount_;  // 0.70..0.98 feedback
        const float damp = 0.5f;
        for (int i = 0; i < kCombs; ++i)
        {
            combL_[i].feedback = room; combL_[i].damp = damp;
            combR_[i].feedback = room; combR_[i].damp = damp;
        }
    }

    void processStereo(float* left, float* right, int numSamples) noexcept
    {
        if (amount_ <= 0.0001f)
            return;
        const float dry = 1.0f - wet_;
        for (int n = 0; n < numSamples; ++n)
        {
            const float input = (left[n] + right[n]) * kGain;
            float outL = 0.0f, outR = 0.0f;
            for (int i = 0; i < kCombs; ++i) { outL += combL_[i].process(input); outR += combR_[i].process(input); }
            for (int i = 0; i < kAllpasses; ++i) { outL = apL_[i].process(outL); outR = apR_[i].process(outR); }
            left[n]  = left[n]  * dry + outL * wet_;
            right[n] = right[n] * dry + outR * wet_;
        }
    }

private:
    static constexpr int kCombs = 8;
    static constexpr int kAllpasses = 4;
    static constexpr int kStereoSpread = 23;
    static constexpr float kGain = 0.015f;
    static constexpr int kCombMax = 3700;   // ceil(1617 * 96000/44100) + spread
    static constexpr int kAllpassMax = 1300;
    // Standard Freeverb tunings (Schroeder, 44100 Hz reference).
    static constexpr int kCombTuning[kCombs] = {1116, 1188, 1277, 1356, 1422, 1491, 1557, 1617};
    static constexpr int kAllpassTuning[kAllpasses] = {556, 441, 341, 225};

    struct Comb {
        std::array<float, kCombMax> buf{};
        int size = 1, idx = 0;
        float feedback = 0.84f, damp = 0.5f, store = 0.0f;
        void setSize(int tuning, double scale)
        {
            size = std::clamp((int) std::lround(tuning * scale), 1, kCombMax);
        }
        void reset() { buf.fill(0.0f); idx = 0; store = 0.0f; }
        float process(float input) noexcept
        {
            const float out = buf[(size_t) idx];
            store = out * (1.0f - damp) + store * damp + 1.0e-25f; // anti-denormal
            buf[(size_t) idx] = input + store * feedback;
            if (++idx >= size) idx = 0;
            return out;
        }
    };

    struct Allpass {
        std::array<float, kAllpassMax> buf{};
        int size = 1, idx = 0;
        float feedback = 0.5f;
        void setSize(int tuning, double scale)
        {
            size = std::clamp((int) std::lround(tuning * scale), 1, kAllpassMax);
        }
        void reset() { buf.fill(0.0f); idx = 0; }
        float process(float input) noexcept
        {
            const float bufout = buf[(size_t) idx];
            const float out = -input + bufout;
            buf[(size_t) idx] = input + bufout * feedback;
            if (++idx >= size) idx = 0;
            return out;
        }
    };

    double sampleRate_ = 48000.0;
    float  amount_ = 0.0f, wet_ = 0.0f;
    std::array<Comb, kCombs> combL_, combR_;
    std::array<Allpass, kAllpasses> apL_, apR_;
};

} // namespace veyra::dsp
