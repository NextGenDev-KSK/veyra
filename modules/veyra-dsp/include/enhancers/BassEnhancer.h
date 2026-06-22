#pragma once

// Adaptive (psychoacoustic) bass. Instead of boosting the fundamental — which
// eats headroom and clips — it isolates the low band, runs it through an
// asymmetric nonlinearity to synthesise harmonics (an octave up and beyond),
// high-passes those harmonics (dropping the DC + fundamental the generator
// leaves behind) and mixes them back. The ear reconstructs the "missing
// fundamental", so bass reads as punchier on small drivers without extra
// low-frequency energy. Header-only, RT-safe. amount 0..1 (0 = bypass).

#include <algorithm>
#include <cmath>

#include "eq/Biquad.h"

namespace veyra::dsp {

class BassEnhancer {
public:
    void prepare(double sampleRate) noexcept
    {
        const double fs = sampleRate > 0 ? sampleRate : 48000.0;
        const auto lp = makeLowPass(fs, 100.0, 0.707);
        const auto hp = makeHighPass(fs, 80.0, 0.707);
        lpL_.setCoeffs(lp); lpR_.setCoeffs(lp);
        hpL_.setCoeffs(hp); hpR_.setCoeffs(hp);
        reset();
    }

    void reset() noexcept { lpL_.reset(); lpR_.reset(); hpL_.reset(); hpR_.reset(); }

    void setAmount(float a) noexcept { amount_ = std::clamp(a, 0.0f, 1.0f); }

    void processStereo(float* left, float* right, int numSamples) noexcept
    {
        if (amount_ <= 0.001f)
            return;
        const float drive = 1.0f + amount_ * 6.0f;
        const float mix   = amount_ * 0.7f;
        const float bias  = 0.1f;
        const float deBias = std::tanh(bias * drive); // cancel the generator's DC
        for (int i = 0; i < numSamples; ++i)
        {
            const float loL = lpL_.process(left[i]);
            const float loR = lpR_.process(right[i]);
            const float gL = std::tanh((loL + bias) * drive) - deBias;
            const float gR = std::tanh((loR + bias) * drive) - deBias;
            left[i]  += mix * hpL_.process(gL); // keep harmonics, drop DC/fundamental
            right[i] += mix * hpR_.process(gR);
        }
    }

private:
    Biquad lpL_, lpR_, hpL_, hpR_;
    float  amount_ = 0.0f;
};

} // namespace veyra::dsp
