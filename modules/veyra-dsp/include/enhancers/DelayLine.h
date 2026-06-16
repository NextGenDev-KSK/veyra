#pragma once

// Integer-sample stereo delay line. For Sound Sharing (multi-output) each
// secondary device is delayed so its playback aligns with the highest-latency
// endpoint — the service measures the per-device latency and sets each route's
// delay to (maxLatency - thisLatency). Allocation happens in prepare(); the
// delay is changed and processed lock-free.

#include <algorithm>
#include <cmath>
#include <vector>

namespace veyra::dsp {

class DelayLine {
public:
    void prepare(double sampleRate, float maxDelayMs) noexcept
    {
        sampleRate_ = sampleRate;
        const int maxSamples = std::max(1, static_cast<int>(std::ceil(maxDelayMs * 0.001 * sampleRate)) + 1);
        bufL_.assign(static_cast<size_t>(maxSamples), 0.0f);
        bufR_.assign(static_cast<size_t>(maxSamples), 0.0f);
        reset();
    }

    void reset() noexcept
    {
        std::fill(bufL_.begin(), bufL_.end(), 0.0f);
        std::fill(bufR_.begin(), bufR_.end(), 0.0f);
        pos_ = 0;
    }

    void setDelayMs(float ms) noexcept
    {
        const int cap = static_cast<int>(bufL_.size());
        delay_ = std::clamp(static_cast<int>(std::lround(ms * 0.001 * sampleRate_)), 0, cap - 1);
    }

    int delaySamples() const noexcept { return delay_; }

    void processStereo(float* left, float* right, int numSamples) noexcept
    {
        if (delay_ == 0 || bufL_.empty())
            return; // aligned output: passthrough

        const int cap = static_cast<int>(bufL_.size());
        for (int i = 0; i < numSamples; ++i)
        {
            const int readPos = (pos_ - delay_ + cap) % cap;
            bufL_[(size_t) pos_] = left[i];
            bufR_[(size_t) pos_] = right[i];
            left[i] = bufL_[(size_t) readPos];
            right[i] = bufR_[(size_t) readPos];
            pos_ = (pos_ + 1) % cap;
        }
    }

private:
    double sampleRate_ = 48000.0;
    int    delay_ = 0;
    int    pos_ = 0;
    std::vector<float> bufL_, bufR_;
};

} // namespace veyra::dsp
