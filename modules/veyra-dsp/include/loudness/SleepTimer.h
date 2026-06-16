#pragma once

// Sleep-timer controller: a pure, time-driven state machine that produces the
// master output gain for the sleep feature. The service ticks advance() with
// the elapsed wall-clock delta and publishes the returned gain; the audio path
// applies it (smoothed) via SleepFade / the chain's master gain. Kept free of
// threads and I/O so the fade schedule is deterministically unit-testable.

#include <algorithm>
#include <cmath>

namespace veyra::dsp {

class SleepTimer {
public:
    // total = minutes until silence; fade = length of the closing fade tail.
    void configure(float totalMinutes, float fadeSeconds) noexcept
    {
        totalSec_ = std::max(0.0, (double) totalMinutes * 60.0);
        fadeSec_  = std::clamp((double) fadeSeconds, 0.5, std::max(0.5, totalSec_));
    }

    void start() noexcept
    {
        elapsed_ = 0.0;
        gain_ = 1.0f;
        armed_ = true;
        finished_ = false;
    }

    void stop() noexcept
    {
        armed_ = false;
        finished_ = false;
        gain_ = 1.0f;
        elapsed_ = 0.0;
    }

    bool  armed() const noexcept { return armed_; }
    bool  finished() const noexcept { return finished_; }
    float gain() const noexcept { return gain_; }

    // Advance by dtSeconds; returns the current master gain (1 -> 0).
    float advance(double dtSeconds) noexcept
    {
        if (!armed_)
            return gain_;

        elapsed_ += dtSeconds;
        const double fadeStart = totalSec_ - fadeSec_;

        if (elapsed_ <= fadeStart)
        {
            gain_ = 1.0f;
        }
        else if (elapsed_ >= totalSec_)
        {
            gain_ = 0.0f;
            finished_ = true;
            armed_ = false;
        }
        else
        {
            const double t = (elapsed_ - fadeStart) / fadeSec_; // 0..1
            gain_ = (float) std::exp(std::log(kFloor) * t);      // 1 -> ~ -60 dB
        }
        return gain_;
    }

private:
    static constexpr double kFloor = 0.001; // -60 dB at the end of the fade

    double totalSec_ = 1800.0; // 30 min
    double fadeSec_ = 20.0;
    double elapsed_ = 0.0;
    float  gain_ = 1.0f;
    bool   armed_ = false;
    bool   finished_ = false;
};

} // namespace veyra::dsp
