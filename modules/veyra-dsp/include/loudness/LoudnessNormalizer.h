// Loudness-match mode: measures the program's short-term loudness (BS.1770) and
// applies a slowly-slewed make-up gain to bring it toward a target LUFS (e.g.
// -14 for streaming parity). The gain is clamped and eased per sample so it
// matches level without pumping. amount/enabled gate it; disabled = passthrough.
#pragma once

#include <algorithm>
#include <cmath>

#include "loudness/LoudnessMeter.h"

namespace veyra::dsp {

class LoudnessNormalizer {
public:
    void prepare(double sampleRate)
    {
        meter_.prepare(sampleRate);
        meter_.setIntegrating(false); // only short-term needed; keeps it bounded
        slew_ = 1.0f - std::exp(-1.0f / (float) (0.4 * sampleRate)); // ~0.4 s
        reset();
    }

    void reset()
    {
        meter_.reset();
        gain_ = 1.0f;
    }

    void setEnabled(bool e) noexcept { enabled_ = e; }
    void setTargetLufs(float lufs) noexcept { target_ = lufs; }
    float currentGain() const noexcept { return gain_; }

    void processStereo(float* left, float* right, int numSamples)
    {
        if (!enabled_)
            return;

        // Measure the (pre-gain) program loudness, then move the make-up gain
        // toward the level that lands the output on target.
        meter_.processStereo(left, right, numSamples);

        float desired = gain_;
        const float st = meter_.shortTermLufs();
        if (st > LoudnessMeter::kSilence + 1.0f)
        {
            const float errDb = target_ - st;                       // dB to add
            desired = std::clamp(std::pow(10.0f, errDb / 20.0f), 0.25f, 4.0f);
        }

        for (int i = 0; i < numSamples; ++i)
        {
            gain_ += (desired - gain_) * slew_;
            left[i]  *= gain_;
            right[i] *= gain_;
        }
    }

private:
    LoudnessMeter meter_;
    bool  enabled_ = false;
    float target_ = -14.0f;
    float gain_ = 1.0f;
    float slew_ = 0.0f;
};

} // namespace veyra::dsp
