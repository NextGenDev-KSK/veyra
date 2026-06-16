#pragma once

// Headphone crossfeed (Bauer-style): a small amount of each channel is delayed
// (inter-aural time difference) and low-passed (head shadow) into the opposite
// channel. This relaxes the hard left/right separation of headphones into a
// more speaker-like, fatigue-free image. A single 0..1 amount controls depth;
// 0 is a bit-exact bypass.

#include <algorithm>
#include <cmath>
#include <vector>

namespace veyra::dsp {

class Crossfeed {
public:
    void prepare(double sampleRate) noexcept
    {
        sampleRate_ = sampleRate;
        delay_ = std::max(1, static_cast<int>(std::lround(0.00027 * sampleRate))); // ~0.27 ms
        bufL_.assign(static_cast<size_t>(delay_), 0.0f);
        bufR_.assign(static_cast<size_t>(delay_), 0.0f);

        const double fc = 900.0; // head-shadow low-pass
        lpA_ = static_cast<float>(std::exp(-2.0 * kPi * fc / sampleRate));
        reset();
    }

    void reset() noexcept
    {
        std::fill(bufL_.begin(), bufL_.end(), 0.0f);
        std::fill(bufR_.begin(), bufR_.end(), 0.0f);
        lpL_ = lpR_ = 0.0f;
        pos_ = 0;
    }

    void setAmount(float amount) noexcept { amount_ = std::clamp(amount, 0.0f, 1.0f); }
    float amount() const noexcept { return amount_; }

    void processStereo(float* left, float* right, int numSamples) noexcept
    {
        if (amount_ <= 0.0f || bufL_.empty())
            return;

        const float g = amount_ * 0.5f;
        for (int i = 0; i < numSamples; ++i)
        {
            const float dR = bufR_[(size_t) pos_]; // delayed opposite channels
            const float dL = bufL_[(size_t) pos_];
            bufL_[(size_t) pos_] = left[i];
            bufR_[(size_t) pos_] = right[i];
            pos_ = (pos_ + 1) % delay_;

            lpL_ = dR + lpA_ * (lpL_ - dR); // low-passed bleed of R into L
            lpR_ = dL + lpA_ * (lpR_ - dL);

            const float outL = (1.0f - g) * left[i] + g * lpL_;
            const float outR = (1.0f - g) * right[i] + g * lpR_;
            left[i] = outL;
            right[i] = outR;
        }
    }

private:
    static constexpr double kPi = 3.14159265358979323846;

    double sampleRate_ = 48000.0;
    float  amount_ = 0.0f;
    int    delay_ = 1;
    int    pos_ = 0;
    float  lpA_ = 0.0f, lpL_ = 0.0f, lpR_ = 0.0f;
    std::vector<float> bufL_, bufR_;
};

} // namespace veyra::dsp
