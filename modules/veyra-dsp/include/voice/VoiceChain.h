#pragma once

// The microphone (capture) processing chain, applied to a mono mic stream:
//
//   high-pass (rumble/plosive) -> noise suppression -> leveling compressor
//   -> de-esser -> presence EQ -> output gain
//
// Allocation- and lock-free; mirrors the render-side DspChain so the capture
// APO can run it directly from a shared-memory parameter snapshot. A single
// VoiceParams struct drives it; setParams() recomputes the filter coefficients.

#include <algorithm>
#include <cmath>

#include "dynamics/Compressor.h"
#include "eq/Biquad.h"
#include "voice/NoiseSuppressor.h"

namespace veyra::dsp {

struct VoiceParams {
    bool  enabled          = true;
    float highPassHz       = 80.0f;  // rumble cut
    float noiseSuppression = 0.5f;   // 0..1
    float compressionAmount = 0.3f;  // 0..1 leveling
    float deEssAmount      = 0.3f;   // 0..1 sibilance taming
    float presenceDb       = 2.0f;   // clarity peak around 3.5 kHz
    float outputGainDb     = 0.0f;   // make-up
    float sideToneLevel    = 0.0f;   // 0..1 — monitored back to the user (routed by the APO)
};

class VoiceChain {
public:
    void prepare(double sampleRate) noexcept
    {
        sampleRate_ = sampleRate;
        comp_.prepare(sampleRate);
        ns_.prepare(sampleRate);

        deEssAtk_ = timeToCoeff(2.0f);
        deEssRel_ = timeToCoeff(40.0f);

        setParams(params_);
        reset();
    }

    void reset() noexcept
    {
        hp_.reset();
        deHp_.reset();
        presence_.reset();
        ns_.reset();
        deEnv_  = 0.0f;
        deGain_ = 1.0f;
    }

    void setParams(const VoiceParams& p) noexcept
    {
        params_ = p;

        const double fs = sampleRate_;
        hp_.setCoeffs(makeHighPass(fs, std::clamp(p.highPassHz, 20.0f, 400.0f), 0.707));
        presence_.setCoeffs(makePeaking(fs, 3500.0, 1.0, p.presenceDb));
        deHp_.setCoeffs(makeHighPass(fs, 5500.0, 0.707));

        ns_.setAmount(p.noiseSuppression);
        comp_.setAmount(p.compressionAmount);

        // De-esser detection band scaled by amount.
        deThreshDb_ = -20.0f - p.deEssAmount * 28.0f; // amount 1 -> -48 dB (sensitive)
        deActive_   = p.deEssAmount > 0.0f;

        outGain_ = std::pow(10.0f, p.outputGainDb / 20.0f);
    }

    const VoiceParams& params() const noexcept { return params_; }

    void processMono(float* x, int numSamples) noexcept
    {
        if (!params_.enabled)
            return;

        // 1. High-pass (rumble / plosives).
        for (int i = 0; i < numSamples; ++i)
            x[i] = hp_.process(x[i]);

        // 2. Noise suppression.
        ns_.processMono(x, numSamples);

        // 3. Leveling compressor.
        comp_.processMono(x, numSamples);

        // 4. De-esser: subtract part of the sibilance band when it's hot.
        if (deActive_)
            processDeEss(x, numSamples);

        // 5. Presence EQ + 6. output gain.
        for (int i = 0; i < numSamples; ++i)
            x[i] = presence_.process(x[i]) * outGain_;
    }

private:
    float timeToCoeff(float ms) const noexcept
    {
        const double t = (ms * 0.001) * sampleRate_;
        return t > 0.0 ? static_cast<float>(std::exp(-1.0 / t)) : 0.0f;
    }

    void processDeEss(float* x, int numSamples) noexcept
    {
        for (int i = 0; i < numSamples; ++i)
        {
            const float hb = deHp_.process(x[i]); // sibilance band
            const float ahb = std::fabs(hb);
            const float coeff = ahb > deEnv_ ? deEssAtk_ : deEssRel_;
            deEnv_ = ahb + coeff * (deEnv_ - ahb);

            const float envDb = 20.0f * std::log10(std::max(deEnv_, 1.0e-9f));
            float target = 1.0f;
            if (envDb > deThreshDb_)
            {
                const float reductDb = std::min((envDb - deThreshDb_) * 0.5f, 12.0f);
                target = std::pow(10.0f, -reductDb / 20.0f);
            }
            const float gc = target < deGain_ ? deEssAtk_ : deEssRel_;
            deGain_ = target + gc * (deGain_ - target);

            // Remove the attenuated portion of the high band.
            x[i] = x[i] - (1.0f - deGain_) * hb;
        }
    }

    double sampleRate_ = 48000.0;
    VoiceParams params_;

    Biquad          hp_, presence_, deHp_;
    NoiseSuppressor ns_;
    Compressor      comp_;

    bool  deActive_   = true;
    float deThreshDb_ = -40.0f;
    float deEssAtk_ = 0.0f, deEssRel_ = 0.0f;
    float deEnv_ = 0.0f, deGain_ = 1.0f;
    float outGain_ = 1.0f;
};

} // namespace veyra::dsp
