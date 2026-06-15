#pragma once

// A minimum-interval gate. The per-app engine uses it so rapid focus changes
// can't thrash the DSP (re-publishing presets dozens of times a second).
// Time is passed in explicitly so it is deterministic to unit-test.

#include <chrono>

namespace veyra {

class RateLimiter {
public:
    using Clock     = std::chrono::steady_clock;
    using TimePoint = Clock::time_point;

    explicit RateLimiter(std::chrono::milliseconds minInterval) : interval_(minInterval) {}

    // Returns true if enough time has elapsed since the last allowed call (and
    // records 'now' as the new reference); false to suppress the action.
    bool allow(TimePoint now)
    {
        if (!primed_ || now - last_ >= interval_)
        {
            last_   = now;
            primed_ = true;
            return true;
        }
        return false;
    }

    void reset() { primed_ = false; }

private:
    std::chrono::milliseconds interval_;
    TimePoint                 last_{};
    bool                      primed_ = false;
};

} // namespace veyra
