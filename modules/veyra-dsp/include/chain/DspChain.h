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
#include "enhancers/StereoProcessor.h"
#include "enhancers/ToneControls.h"
#include "eq/GraphicEq.h"
#include "loudness/LoudnessNormalizer.h"
#include "loudness/NightMode.h"
#include "spatial/Crossfeed.h"
#include "spatial/VirtualSurround.h"

namespace veyra::dsp {

class DspChain {
public:
    void prepare(double sampleRate, int maxBlock = 0)
    {
        eq_.prepare(sampleRate, maxBlock);
        tone_.prepare(sampleRate);
        stereo_.prepare(sampleRate);
        comp_.prepare(sampleRate);
        crossfeed_.prepare(sampleRate);
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
        for (int b = 0; b < GraphicEq::kNumBands; ++b)
            eq_.setBandGainDb(b, p.eqBandsDb[static_cast<size_t>(b)]);
        tone_.setBassDb(p.bassBoostDb);
        tone_.setTrebleDb(p.trebleDb);
        stereo_.setMono(p.monoMode);
        stereo_.setBalance(p.balance);
        stereo_.setWidth(p.stereoWidth);
        comp_.setAmount(p.compressionAmount);
        crossfeed_.setAmount(p.crossfeedAmount);
        surround_.setAmount(p.virtualizationAmount);
        nightMode_.setAmount(p.nightModeAmount);
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

        stereo_.applyMonoBalance(left, right, numSamples);
        eq_.processStereo(left, right, numSamples);
        tone_.processStereo(left, right, numSamples);
        comp_.processStereo(left, right, numSamples);
        stereo_.applyWidth(left, right, numSamples);
        surround_.processStereo(left, right, numSamples);  // HRTF virtualisation
        crossfeed_.processStereo(left, right, numSamples);  // headphone crossfeed
        nightMode_.processStereo(left, right, numSamples);  // late-night loudness

        for (int i = 0; i < numSamples; ++i)
        {
            const float g = volume_.next();
            left[i]  *= g;
            right[i] *= g;
        }

        normalizer_.processStereo(left, right, numSamples); // EBU R128 loudness match
        limiter_.processStereo(left, right, numSamples);
        analyzer_.processStereo(left, right, numSamples);
    }

    bool popAnalyzerFrame(AnalyzerFrame& out) noexcept { return analyzer_.popFrame(out); }
    int  latencySamples() const noexcept { return limiter_.latencySamples(); }

private:
    bool bypass_ = false;
    GraphicEq        eq_;
    ToneControls     tone_;
    StereoProcessor  stereo_;
    Compressor       comp_;
    Crossfeed        crossfeed_;
    VirtualSurround  surround_;
    NightMode        nightMode_;
    LoudnessNormalizer normalizer_;
    Limiter          limiter_;
    Analyzer         analyzer_;
    SmoothedValue    volume_;
};

} // namespace veyra::dsp
