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
    bool  agc              = false;  // automatic gain control (level toward target)
    float agcTargetDb      = -16.0f; // AGC target RMS (≈ -16 LUFS)
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

        agcEnvCoeff_ = timeToCoeff(50.0f);  // power smoothing
        agcAtk_      = timeToCoeff(200.0f); // gain rise
        agcRel_      = timeToCoeff(600.0f); // gain fall

        setParams(params_);
        reset();
    }

    void reset() noexcept
    {
        hp_.reset();
        deHp_.reset();
        deShelf_.reset();
        presence_.reset();
        ns_.reset();
        deEnv_      = 0.0f;
        deReductDb_ = 0.0f;
        lastShelfDb_ = -1.0f; // force a coeff refresh
        agcEnv_ = 0.0f;
        agcGain_ = 1.0f;
    }

    void setParams(const VoiceParams& p) noexcept
    {
        params_ = p;

        const double fs = sampleRate_;
        hp_.setCoeffs(makeHighPass(fs, std::clamp(p.highPassHz, 20.0f, 400.0f), 0.707));
        presence_.setCoeffs(makePeaking(fs, 3500.0, 1.0, p.presenceDb));
        deHp_.setCoeffs(makeHighPass(fs, kDeEssHz, 0.707));        // sibilance detector band
        deShelf_.setCoeffs(makeHighShelf(fs, kDeEssHz, 0.707, 0.0)); // dynamic reducer
        lastShelfDb_ = 0.0f;

        ns_.setAmount(p.noiseSuppression);
        comp_.setAmount(p.compressionAmount);

        agcActive_     = p.agc;
        agcTargetLin_  = std::pow(10.0f, p.agcTargetDb / 20.0f);

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

        // 4.5 AGC: slowly level toward the target RMS.
        if (agcActive_)
            processAgc(x, numSamples);

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
            // Detect sibilance energy (magnitude only — the band's phase is
            // irrelevant for detection).
            const float ahb = std::fabs(deHp_.process(x[i]));
            const float ecoeff = ahb > deEnv_ ? deEssAtk_ : deEssRel_;
            deEnv_ = ahb + ecoeff * (deEnv_ - ahb);

            const float envDb = 20.0f * std::log10(std::max(deEnv_, 1.0e-9f));
            float targetReductDb = 0.0f;
            if (envDb > deThreshDb_)
                targetReductDb = std::min((envDb - deThreshDb_) * 0.5f, 12.0f);

            const float rc = targetReductDb > deReductDb_ ? deEssAtk_ : deEssRel_;
            deReductDb_ = targetReductDb + rc * (deReductDb_ - targetReductDb);

            // Apply as a phase-correct dynamic high-shelf cut. Recompute coeffs
            // only when the (quantised) reduction actually moves.
            const float q = std::round(deReductDb_ * 2.0f) * 0.5f;
            if (q != lastShelfDb_)
            {
                deShelf_.setCoeffs(makeHighShelf(sampleRate_, kDeEssHz, 0.707, -q));
                lastShelfDb_ = q;
            }
            x[i] = deShelf_.process(x[i]);
        }
    }

    void processAgc(float* x, int numSamples) noexcept
    {
        constexpr float kFloorPow = 1.0e-8f; // ~ -80 dBFS: hold gain in silence
        for (int i = 0; i < numSamples; ++i)
        {
            const float pw = x[i] * x[i];
            agcEnv_ = pw + agcEnvCoeff_ * (agcEnv_ - pw);
            float desired = agcGain_;
            if (agcEnv_ > kFloorPow)
            {
                const float rms = std::sqrt(agcEnv_);
                desired = std::clamp(agcTargetLin_ / (rms + 1.0e-9f), 0.25f, 4.0f); // ±12 dB
            }
            const float sc = desired > agcGain_ ? agcAtk_ : agcRel_;
            agcGain_ = desired + sc * (agcGain_ - desired);
            x[i] *= agcGain_;
        }
    }

    static constexpr double kDeEssHz = 5500.0;

    double sampleRate_ = 48000.0;
    VoiceParams params_;

    Biquad          hp_, presence_, deHp_, deShelf_;
    NoiseSuppressor ns_;
    Compressor      comp_;

    bool  deActive_   = true;
    float deThreshDb_ = -40.0f;
    float deEssAtk_ = 0.0f, deEssRel_ = 0.0f;
    float deEnv_ = 0.0f, deReductDb_ = 0.0f, lastShelfDb_ = -1.0f;
    float outGain_ = 1.0f;

    bool  agcActive_ = false;
    float agcTargetLin_ = 0.158f; // -16 dB
    float agcEnv_ = 0.0f, agcGain_ = 1.0f;
    float agcEnvCoeff_ = 0.0f, agcAtk_ = 0.0f, agcRel_ = 0.0f;
};

} // namespace veyra::dsp
