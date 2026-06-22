#pragma once

// Cinematic room simulation: a compact early-reflection network that places the
// sound in a small room for out-of-head depth and envelopment (the precursor a
// reverb tail then extends). A few tapped delays per channel, cross-fed L<->R so
// reflections arrive at both ears with a spread, mixed in by amount. This is the
// directional "speaker in a room" cue that plain headphone playback lacks.
// Header-only, RT-safe, allocation-free after prepare(). amount 0..1 (0 = bypass).

#include <algorithm>
#include <array>
#include <cmath>
#include <vector>

namespace veyra::dsp {

class RoomSimulator {
public:
    void prepare(double sampleRate) noexcept
    {
        sampleRate_ = sampleRate > 0 ? sampleRate : 48000.0;
        size_ = std::max(1, (int) std::lround(0.06 * sampleRate_)); // 60 ms ring
        bufL_.assign((size_t) size_, 0.0f);
        bufR_.assign((size_t) size_, 0.0f);
        // Early-reflection pattern (milliseconds, gain) — irregular spacing avoids
        // a metallic comb; gains decay with arrival time.
        const std::array<std::pair<double, float>, 6> ms = {{
            {7.0, 0.70f}, {11.0, 0.60f}, {17.0, 0.50f},
            {23.0, 0.42f}, {31.0, 0.34f}, {41.0, 0.26f}}};
        nTaps_ = 0;
        for (const auto& [t, g] : ms)
        {
            const int d = (int) std::lround(t * 0.001 * sampleRate_);
            if (d > 0 && d < size_) { tapDelay_[nTaps_] = d; tapGain_[nTaps_] = g; ++nTaps_; }
        }
        reset();
    }

    void reset() noexcept
    {
        std::fill(bufL_.begin(), bufL_.end(), 0.0f);
        std::fill(bufR_.begin(), bufR_.end(), 0.0f);
        pos_ = 0;
    }

    void setAmount(float a) noexcept { amount_ = std::clamp(a, 0.0f, 1.0f); }

    void processStereo(float* left, float* right, int numSamples) noexcept
    {
        if (amount_ <= 0.001f || size_ <= 1)
            return;
        const float wet = amount_ * 0.5f;
        for (int i = 0; i < numSamples; ++i)
        {
            float erL = 0.0f, erR = 0.0f;
            for (int t = 0; t < nTaps_; ++t)
            {
                int idx = pos_ - tapDelay_[t];
                if (idx < 0) idx += size_;
                const float dl = bufL_[(size_t) idx];
                const float dr = bufR_[(size_t) idx];
                // mostly-contralateral arrival (0.65 cross / 0.35 same) for width
                erL += tapGain_[t] * (0.35f * dl + 0.65f * dr);
                erR += tapGain_[t] * (0.35f * dr + 0.65f * dl);
            }
            bufL_[(size_t) pos_] = left[i];
            bufR_[(size_t) pos_] = right[i];
            pos_ = (pos_ + 1) % size_;
            left[i]  += wet * erL;
            right[i] += wet * erR;
        }
    }

private:
    double sampleRate_ = 48000.0;
    float  amount_ = 0.0f;
    int    size_ = 1, pos_ = 0, nTaps_ = 0;
    std::array<int, 6>   tapDelay_{};
    std::array<float, 6> tapGain_{};
    std::vector<float> bufL_, bufR_;
};

} // namespace veyra::dsp
