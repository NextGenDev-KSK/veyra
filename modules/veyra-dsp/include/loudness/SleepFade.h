#pragma once

// Sleep timer fade-out: a smooth exponential gain ramp from unity to silence
// over a set duration, applied to the output. Exponential (constant dB/sec)
// matches how loudness is perceived, so the fade feels linear to the ear.
// Inactive (or cancelled) it is a bit-exact passthrough.

#include <algorithm>
#include <cmath>

namespace veyra::dsp {

class SleepFade {
public:
    void prepare(double sampleRate) noexcept
    {
        sampleRate_ = sampleRate;
        cancel();
    }

    // Begin a fade to silence over 'durationSec'.
    void start(float durationSec) noexcept
    {
        const double dur = std::max(0.05, (double) durationSec);
        // Reach kFloor (~ -60 dB) at the end, then snap to zero.
        decay_ = (float) std::exp(std::log(kFloor) / (dur * sampleRate_));
        gain_ = 1.0f;
        active_ = true;
        finished_ = false;
    }

    void cancel() noexcept
    {
        gain_ = 1.0f;
        decay_ = 1.0f;
        active_ = false;
        finished_ = false;
    }

    bool  active() const noexcept { return active_; }
    bool  finished() const noexcept { return finished_; }
    float currentGain() const noexcept { return gain_; }

    void processStereo(float* left, float* right, int numSamples) noexcept
    {
        if (!active_)
            return;
        for (int i = 0; i < numSamples; ++i)
        {
            left[i] *= gain_;
            right[i] *= gain_;
            gain_ *= decay_;
            if (gain_ <= kFloor)
            {
                gain_ = 0.0f;
                finished_ = true;
            }
        }
    }

private:
    static constexpr float kFloor = 0.001f; // -60 dB

    double sampleRate_ = 48000.0;
    float  gain_ = 1.0f;
    float  decay_ = 1.0f;
    bool   active_ = false;
    bool   finished_ = false;
};

} // namespace veyra::dsp
