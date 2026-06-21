#pragma once

// Parametric EQ: up to 16 independent bands, each a biquad of a selectable type
// (bell / low-shelf / high-shelf / notch / high-pass / low-pass) with its own
// frequency, gain and Q. Header-only, allocation-free, RT-safe — coefficients
// are recomputed only when a band changes, never in the audio loop.

#include <algorithm>
#include <array>
#include <cmath>

#include "eq/Biquad.h"

namespace veyra::dsp {

enum class EqBandType { Bell, LowShelf, HighShelf, Notch, HighPass, LowPass };

struct EqBand {
    bool       enabled = false;
    EqBandType type    = EqBandType::Bell;
    float      freq    = 1000.0f;
    float      gainDb  = 0.0f;
    float      q       = 1.0f;
};

class ParametricEq {
public:
    static constexpr int kMaxBands = 16;

    void prepare(double sampleRate) noexcept
    {
        sampleRate_ = sampleRate > 0.0 ? sampleRate : 48000.0;
        for (int b = 0; b < kMaxBands; ++b) { left_[b].reset(); right_[b].reset(); }
        recompute();
    }

    void setEnabled(bool e) noexcept { enabled_ = e; }
    bool enabled() const noexcept { return enabled_; }

    void setBands(const std::array<EqBand, kMaxBands>& bands) noexcept
    {
        bands_ = bands;
        recompute();
    }

    void setBand(int i, const EqBand& b) noexcept
    {
        if (i >= 0 && i < kMaxBands) { bands_[(size_t) i] = b; recomputeBand(i); }
    }

    void processStereo(float* left, float* right, int numSamples) noexcept
    {
        if (!enabled_)
            return;
        for (int i = 0; i < numSamples; ++i)
        {
            float xl = left[i], xr = right[i];
            for (int b = 0; b < kMaxBands; ++b)
                if (active_[(size_t) b])
                {
                    xl = left_[(size_t) b].process(xl);
                    xr = right_[(size_t) b].process(xr);
                }
            left[i]  = xl;
            right[i] = xr;
        }
    }

private:
    void recompute() noexcept { for (int b = 0; b < kMaxBands; ++b) recomputeBand(b); }

    void recomputeBand(int b) noexcept
    {
        const auto& bd = bands_[(size_t) b];
        active_[(size_t) b] = bd.enabled;
        if (!bd.enabled)
            return;
        const double f = std::clamp((double) bd.freq, 20.0, sampleRate_ * 0.45);
        const double q = std::max(0.1f, bd.q);
        BiquadCoeffs c;
        switch (bd.type)
        {
        case EqBandType::Bell:      c = makePeaking(sampleRate_, f, q, bd.gainDb); break;
        case EqBandType::LowShelf:  c = makeLowShelf(sampleRate_, f, q, bd.gainDb); break;
        case EqBandType::HighShelf: c = makeHighShelf(sampleRate_, f, q, bd.gainDb); break;
        case EqBandType::Notch:     c = makeNotch(sampleRate_, f, q); break;
        case EqBandType::HighPass:  c = makeHighPass(sampleRate_, f, q); break;
        case EqBandType::LowPass:   c = makeLowPass(sampleRate_, f, q); break;
        }
        left_[(size_t) b].setCoeffs(c);
        right_[(size_t) b].setCoeffs(c);
    }

    double sampleRate_ = 48000.0;
    bool   enabled_ = false;
    std::array<EqBand, kMaxBands> bands_{};
    std::array<bool, kMaxBands>   active_{};
    std::array<Biquad, kMaxBands> left_, right_;
};

} // namespace veyra::dsp
