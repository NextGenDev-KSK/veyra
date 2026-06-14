#pragma once

// Feed-forward, soft-knee, stereo-linked compressor. The detector runs on the
// link peak (max of |L|,|R|); one gain is applied to both channels so the
// stereo image is preserved. Exposes the full parameter set plus a single
// 0..1 "amount" knob (the spec's compression control) that maps onto sensible
// threshold/ratio/makeup values.

#include <algorithm>
#include <cmath>

namespace veyra::dsp {

class Compressor {
public:
    void prepare(double sampleRate) noexcept
    {
        sampleRate_ = sampleRate;
        setAttackMs(attackMs_);
        setReleaseMs(releaseMs_);
        envDb_ = 0.0f;
    }

    void setThresholdDb(float t) noexcept { thresholdDb_ = t; }
    void setRatio(float r) noexcept       { ratio_ = std::max(1.0f, r); }
    void setKneeDb(float k) noexcept      { kneeDb_ = std::max(0.0f, k); }
    void setMakeupDb(float m) noexcept    { makeupDb_ = m; }

    void setAttackMs(float ms) noexcept
    {
        attackMs_ = ms;
        attackCoeff_ = timeToCoeff(ms);
    }
    void setReleaseMs(float ms) noexcept
    {
        releaseMs_ = ms;
        releaseCoeff_ = timeToCoeff(ms);
    }

    // Single-knob control: 0 = transparent, 1 = heavy. Mild auto-makeup.
    void setAmount(float amount) noexcept
    {
        const float a = std::clamp(amount, 0.0f, 1.0f);
        thresholdDb_ = -6.0f - a * 24.0f;      // 0 -> -6 dB, 1 -> -30 dB
        ratio_       = 1.0f + a * 7.0f;        // 0 -> 1:1, 1 -> 8:1
        kneeDb_      = 6.0f;
        makeupDb_    = a * (-thresholdDb_) * 0.3f; // partial gain make-up
    }

    void processStereo(float* left, float* right, int numSamples) noexcept
    {
        for (int i = 0; i < numSamples; ++i)
        {
            const float peak = std::max(std::fabs(left[i]), std::fabs(right[i]));
            const float lvlDb = 20.0f * std::log10(std::max(peak, 1.0e-9f));

            const float targetGainDb = computeGainDb(lvlDb); // <= 0

            // Ballistics: attack when we need more reduction, release otherwise.
            const float coeff = targetGainDb < envDb_ ? attackCoeff_ : releaseCoeff_;
            envDb_ = targetGainDb + coeff * (envDb_ - targetGainDb);

            const float g = std::pow(10.0f, (envDb_ + makeupDb_) / 20.0f);
            left[i]  *= g;
            right[i] *= g;
        }
    }

private:
    float timeToCoeff(float ms) const noexcept
    {
        const double t = (ms * 0.001) * sampleRate_;
        return t > 0.0 ? static_cast<float>(std::exp(-1.0 / t)) : 0.0f;
    }

    // Soft-knee static curve; returns gain to apply in dB (<= 0).
    float computeGainDb(float lvlDb) const noexcept
    {
        const float over = lvlDb - thresholdDb_;
        const float slope = (1.0f / ratio_) - 1.0f; // negative

        if (over <= -kneeDb_ * 0.5f)
            return 0.0f;
        if (over < kneeDb_ * 0.5f)
        {
            const float x = over + kneeDb_ * 0.5f;
            return slope * (x * x) / (2.0f * kneeDb_);
        }
        return slope * over;
    }

    double sampleRate_ = 48000.0;
    float thresholdDb_ = -18.0f;
    float ratio_ = 2.0f;
    float kneeDb_ = 6.0f;
    float makeupDb_ = 0.0f;
    float attackMs_ = 10.0f;
    float releaseMs_ = 120.0f;
    float attackCoeff_ = 0.0f;
    float releaseCoeff_ = 0.0f;
    float envDb_ = 0.0f;
};

} // namespace veyra::dsp
