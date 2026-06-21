#pragma once

// Acoustic Echo Canceller: a normalised LMS (NLMS) adaptive FIR that estimates
// the echo of a far-end reference (what the speakers play) present in the mic and
// subtracts it. Header-only, allocation-free after prepare(), RT-safe (circular
// reference buffer, no per-sample shifting). Wiring the far-end reference into
// the live capture path is a service/APO step; this is the tested algorithm.

#include <algorithm>
#include <cmath>
#include <vector>

namespace veyra::dsp {

class AcousticEchoCanceller {
public:
    void prepare(int taps)
    {
        taps_ = std::max(8, taps);
        w_.assign((size_t) taps_, 0.0f);
        x_.assign((size_t) taps_, 0.0f);
        pos_ = 0;
    }

    void reset()
    {
        std::fill(w_.begin(), w_.end(), 0.0f);
        std::fill(x_.begin(), x_.end(), 0.0f);
        pos_ = 0;
    }

    void setStepSize(float mu) noexcept { mu_ = std::clamp(mu, 0.0f, 1.0f); }

    // near = mic sample (near-end speech + echo of far); far = reference sample.
    // Returns the error signal = near with the estimated echo removed.
    float processSample(float near, float far) noexcept
    {
        x_[(size_t) pos_] = far;

        float y = 0.0f;          // estimated echo
        float energy = 1.0e-6f;  // reference energy (NLMS normaliser)
        int idx = pos_;
        for (int i = 0; i < taps_; ++i)
        {
            const float xi = x_[(size_t) idx];
            y += w_[(size_t) i] * xi;
            energy += xi * xi;
            idx = (idx == 0) ? taps_ - 1 : idx - 1;
        }

        const float e = near - y;
        const float g = mu_ * e / energy;
        idx = pos_;
        for (int i = 0; i < taps_; ++i)
        {
            w_[(size_t) i] += g * x_[(size_t) idx];
            idx = (idx == 0) ? taps_ - 1 : idx - 1;
        }

        pos_ = (pos_ + 1) % taps_;
        return e;
    }

    void processBlock(float* mic, const float* far, int numSamples) noexcept
    {
        for (int i = 0; i < numSamples; ++i)
            mic[i] = processSample(mic[i], far[i]);
    }

private:
    int   taps_ = 0;
    int   pos_  = 0;
    float mu_   = 0.5f;
    std::vector<float> w_, x_;
};

} // namespace veyra::dsp
