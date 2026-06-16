#pragma once

// Night Mode: tames dynamic range for quiet late-night listening — loud peaks
// are compressed and the quiet parts lifted by make-up gain, so dialogue stays
// audible without sudden loud hits. A single 0..1 amount; 0 is a bypass.
// Built on the shared stereo Compressor.

#include <algorithm>

#include "dynamics/Compressor.h"

namespace veyra::dsp {

class NightMode {
public:
    void prepare(double sampleRate) noexcept
    {
        comp_.prepare(sampleRate);
        comp_.setAttackMs(15.0f);
        comp_.setReleaseMs(220.0f);
        setAmount(amount_);
    }

    void setAmount(float amount) noexcept
    {
        amount_ = std::clamp(amount, 0.0f, 1.0f);
        comp_.setThresholdDb(-18.0f - amount_ * 18.0f); // -18 .. -36 dB
        comp_.setRatio(2.0f + amount_ * 4.0f);          // 2:1 .. 6:1
        comp_.setKneeDb(8.0f);
        comp_.setMakeupDb(amount_ * 10.0f);             // lift the quiet parts
    }

    float amount() const noexcept { return amount_; }

    void processStereo(float* left, float* right, int numSamples) noexcept
    {
        if (amount_ <= 0.0f)
            return; // bypass
        comp_.processStereo(left, right, numSamples);
    }

private:
    Compressor comp_;
    float      amount_ = 0.0f;
};

} // namespace veyra::dsp
