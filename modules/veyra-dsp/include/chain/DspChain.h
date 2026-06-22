#pragma once

// The system output processing chain, assembled in the fixed order from the
// spec: mono/balance -> graphic EQ -> bass -> treble -> compressor -> widener
// -> volume -> true-peak limiter -> analyzer tap. (Reverb/echo/HRTF/loudness
// slot in at their later phases.) Everything downstream is allocation-free and
// lock-free, so the whole chain is safe to run from the APO's real-time thread.

#include "analyzer/Analyzer.h"
#include "chain/DspParameters.h"
#include "chain/SmoothedValue.h"
#include "dynamics/Compressor.h"
#include "dynamics/Limiter.h"
#include "enhancers/Reverb.h"
#include "enhancers/StereoProcessor.h"
#include "enhancers/ToneControls.h"
#include "eq/GraphicEq.h"
#include "eq/ParametricEq.h"
#include "enhancers/Exciter.h"
#include "loudness/EqualLoudness.h"
#include "loudness/LoudnessNormalizer.h"
#include "loudness/NightMode.h"
#include "spatial/Crossfeed.h"
#include "spatial/VirtualSurround.h"

namespace veyra::dsp {

class DspChain {
public:
    // Set the MIT KEMAR "diffuse" directory for measured HRTF virtualisation
    // (call before prepare()); empty = synthetic fallback.
    void setHrtfDirectory(std::filesystem::path dir) { hrtfDir_ = std::move(dir); }

    void prepare(double sampleRate, int maxBlock = 0)
    {
        eq_.prepare(sampleRate, maxBlock);
        parametric_.prepare(sampleRate);
        tone_.prepare(sampleRate);
        stereo_.prepare(sampleRate);
        comp_.prepare(sampleRate);
        equalLoudness_.prepare(sampleRate);
        exciter_.prepare(sampleRate);
        reverb_.prepare(sampleRate);
        crossfeed_.prepare(sampleRate);
        surround_.setHrtfDirectory(hrtfDir_);
        surround_.prepare(sampleRate);
        nightMode_.prepare(sampleRate);
        normalizer_.prepare(sampleRate);
        limiter_.prepare(sampleRate);
        analyzer_.prepare(sampleRate);
        volume_.prepare(sampleRate);
        volume_.reset(1.0f);
    }

    void setParameters(const DspParameters& p) noexcept
    {
        bypass_ = p.bypass;
        referenceMode_ = p.referenceMode;
        parametricMode_ = p.parametricMode;
        for (int b = 0; b < GraphicEq::kNumBands; ++b)
            eq_.setBandGainDb(b, p.eqBandsDb[static_cast<size_t>(b)]);

        // Parametric mode reuses the 10 band gains as bell filters at the same
        // centres (the draggable per-node freq/Q editor extends this later).
        static constexpr double kCentres[10] =
            {31.25, 62.5, 125.0, 250.0, 500.0, 1000.0, 2000.0, 4000.0, 8000.0, 16000.0};
        if (parametricMode_ && !explicitParametric_)
        {
            std::array<EqBand, ParametricEq::kMaxBands> bands{};
            for (int b = 0; b < 10; ++b)
                bands[(size_t) b] = EqBand{true, EqBandType::Bell,
                                           (float) kCentres[b], p.eqBandsDb[(size_t) b], 1.41f};
            parametric_.setBands(bands);
        }
        parametric_.setEnabled(parametricMode_);

        tone_.setBassDb(p.bassBoostDb);
        tone_.setTrebleDb(p.trebleDb);
        stereo_.setMono(p.monoMode);
        stereo_.setBalance(p.balance);
        stereo_.setWidth(p.stereoWidth);
        comp_.setAmount(p.compressionAmount);
        reverb_.setAmount(p.reverbAmount);
        crossfeed_.setAmount(p.crossfeedAmount);
        surround_.setAmount(p.virtualizationAmount);
        nightMode_.setAmount(p.nightModeAmount);
        equalLoudness_.setEnabled(p.equalLoudness);
        equalLoudness_.setReference(p.volumeGain);
        exciter_.setAmount(p.exciterAmount);
        normalizer_.setEnabled(p.loudnessMatch);
        normalizer_.setTargetLufs(p.loudnessTargetLufs);
        volume_.setTarget(p.volumeGain);
        limiter_.setCeilingDb(p.limiterCeilingDb);
    }

    void processStereo(float* left, float* right, int numSamples) noexcept
    {
        if (bypass_)
        {
            analyzer_.processStereo(left, right, numSamples);
            return;
        }

        // Reference mode: bypass all coloration (EQ, tone, dynamics, spatial,
        // loudness) and pass the program straight to master volume + the safety
        // limiter, for a flat A/B against the processed sound.
        if (!referenceMode_)
        {
            stereo_.applyMonoBalance(left, right, numSamples);
            if (parametricMode_)
                parametric_.processStereo(left, right, numSamples);
            else
                eq_.processStereo(left, right, numSamples);
            tone_.processStereo(left, right, numSamples);
            comp_.processStereo(left, right, numSamples);
            equalLoudness_.processStereo(left, right, numSamples); // low-volume tonal compensation
            exciter_.processStereo(left, right, numSamples);    // harmonic presence/air
            stereo_.applyWidth(left, right, numSamples);
            reverb_.processStereo(left, right, numSamples);     // ambience (wet/dry)
            surround_.processStereo(left, right, numSamples);  // HRTF virtualisation
            crossfeed_.processStereo(left, right, numSamples);  // headphone crossfeed
            nightMode_.processStereo(left, right, numSamples);  // late-night loudness
        }

        for (int i = 0; i < numSamples; ++i)
        {
            const float g = volume_.next();
            left[i]  *= g;
            right[i] *= g;
        }

        if (!referenceMode_)
            normalizer_.processStereo(left, right, numSamples); // EBU R128 loudness match
        limiter_.processStereo(left, right, numSamples);
        analyzer_.processStereo(left, right, numSamples);
    }

    // Explicit parametric bands (editor-driven). count<=0 reverts to deriving
    // the parametric curve from the 10 band gains in setParameters().
    void setParametricBands(const std::array<EqBand, ParametricEq::kMaxBands>& bands, int count) noexcept
    {
        explicitParametric_ = count > 0;
        if (explicitParametric_)
            parametric_.setBands(bands);
    }

    bool popAnalyzerFrame(AnalyzerFrame& out) noexcept { return analyzer_.popFrame(out); }
    int  latencySamples() const noexcept { return limiter_.latencySamples(); }

private:
    bool bypass_ = false;
    bool referenceMode_ = false;
    bool parametricMode_ = false;
    bool explicitParametric_ = false;
    std::filesystem::path hrtfDir_;
    GraphicEq        eq_;
    ParametricEq     parametric_;
    ToneControls     tone_;
    StereoProcessor  stereo_;
    Compressor       comp_;
    EqualLoudness    equalLoudness_;
    Exciter          exciter_;
    Reverb           reverb_;
    Crossfeed        crossfeed_;
    VirtualSurround  surround_;
    NightMode        nightMode_;
    LoudnessNormalizer normalizer_;
    Limiter          limiter_;
    Analyzer         analyzer_;
    SmoothedValue    volume_;
};

} // namespace veyra::dsp
