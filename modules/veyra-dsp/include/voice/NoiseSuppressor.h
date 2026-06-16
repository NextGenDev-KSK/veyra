#pragma once

// Single-channel noise suppressor for the microphone chain: a downward
// expander / smart gate. A single 0..1 "amount" sets the threshold (in dBFS),
// expansion ratio and maximum attenuation; below the threshold the signal is
// pushed down (steady hiss/hum), while speech above it passes through. Opens
// fast and closes slowly so word tails aren't clipped and it doesn't chatter.
//
// Deterministic, allocation-free and RT-safe. It is the default suppressor and
// also the integration seam for a learned model (RNNoise) behind a build flag
// in a later pass — the chain only depends on processMono(), so the backend can
// be swapped without touching callers.

#include <algorithm>
#include <cmath>

namespace veyra::dsp {

class NoiseSuppressor {
public:
    void prepare(double sampleRate) noexcept
    {
        sampleRate_  = sampleRate;
        envAtkCoeff_ = timeToCoeff(1.0f);
        envRelCoeff_ = timeToCoeff(60.0f);
        gOpenCoeff_  = timeToCoeff(2.0f);
        gCloseCoeff_ = timeToCoeff(80.0f);
        reset();
    }

    void reset() noexcept
    {
        env_  = 0.0f;
        gain_ = 1.0f;
    }

    // 0 = off (transparent), 1 = maximum suppression.
    void setAmount(float amount) noexcept
    {
        amount_      = std::clamp(amount, 0.0f, 1.0f);
        thresholdDb_ = -65.0f + amount_ * 35.0f; // amount 1 -> -30 dBFS gate
        ratio_       = 1.0f + amount_ * 5.0f;     // downward-expansion slope
        maxReductDb_ = amount_ * 32.0f;           // deepest attenuation
    }

    float amount() const noexcept { return amount_; }

    void processMono(float* x, int numSamples) noexcept
    {
        if (amount_ <= 0.0f)
            return; // transparent

        for (int i = 0; i < numSamples; ++i)
        {
            const float ax = std::fabs(x[i]);

            // Peak envelope: fast attack, slower release.
            const float eCoeff = ax > env_ ? envAtkCoeff_ : envRelCoeff_;
            env_ = ax + eCoeff * (env_ - ax);

            const float envDb = 20.0f * std::log10(std::max(env_, 1.0e-9f));

            float targetGain = 1.0f;
            if (envDb < thresholdDb_)
            {
                const float reductDb =
                    std::min((thresholdDb_ - envDb) * (ratio_ - 1.0f), maxReductDb_);
                targetGain = std::pow(10.0f, -reductDb / 20.0f);
            }

            // Open fast, close slowly (preserve word tails, avoid chatter).
            const float gCoeff = targetGain > gain_ ? gOpenCoeff_ : gCloseCoeff_;
            gain_ = targetGain + gCoeff * (gain_ - targetGain);

            x[i] *= gain_;
        }
    }

private:
    float timeToCoeff(float ms) const noexcept
    {
        const double t = (ms * 0.001) * sampleRate_;
        return t > 0.0 ? static_cast<float>(std::exp(-1.0 / t)) : 0.0f;
    }

    double sampleRate_ = 48000.0;
    float  amount_ = 0.0f;
    float  thresholdDb_ = -40.0f, ratio_ = 1.0f, maxReductDb_ = 0.0f;

    float env_ = 0.0f, gain_ = 1.0f;
    float envAtkCoeff_ = 0.0f, envRelCoeff_ = 0.0f;
    float gOpenCoeff_ = 0.0f, gCloseCoeff_ = 0.0f;
};

} // namespace veyra::dsp
