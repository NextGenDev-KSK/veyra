#pragma once

// Bass and treble shelving, matching the spec chain: a low shelf at 80 Hz and a
// high shelf at 8 kHz, each 0..+12 dB (negative also allowed). Smoothed and
// updated at block rate like the graphic EQ.

#include "chain/SmoothedValue.h"
#include "eq/Biquad.h"

namespace veyra::dsp {

class ToneControls {
public:
    void prepare(double sampleRate) noexcept
    {
        sampleRate_ = sampleRate;
        bassDb_.prepare(sampleRate);  bassDb_.reset(0.0f);
        trebleDb_.prepare(sampleRate); trebleDb_.reset(0.0f);
        lastBass_ = lastTreble_ = 0.0f;
        updateBass(0.0f);
        updateTreble(0.0f);
        for (auto* f : {&bassL_, &bassR_, &trebleL_, &trebleR_})
            f->reset();
    }

    void setBassDb(float db) noexcept   { bassDb_.setTarget(db); }
    void setTrebleDb(float db) noexcept { trebleDb_.setTarget(db); }

    void processStereo(float* left, float* right, int numSamples) noexcept
    {
        const float b = bassDb_.skip(numSamples);
        if (std::fabs(b - lastBass_) > 1.0e-4f) { lastBass_ = b; updateBass(b); }

        const float t = trebleDb_.skip(numSamples);
        if (std::fabs(t - lastTreble_) > 1.0e-4f) { lastTreble_ = t; updateTreble(t); }

        for (int i = 0; i < numSamples; ++i)
        {
            left[i]  = trebleL_.process(bassL_.process(left[i]));
            right[i] = trebleR_.process(bassR_.process(right[i]));
        }
    }

private:
    void updateBass(float db) noexcept
    {
        const auto c = makeLowShelf(sampleRate_, 80.0, 0.707, db);
        bassL_.setCoeffs(c);
        bassR_.setCoeffs(c);
    }
    void updateTreble(float db) noexcept
    {
        const auto c = makeHighShelf(sampleRate_, 8000.0, 0.707, db);
        trebleL_.setCoeffs(c);
        trebleR_.setCoeffs(c);
    }

    double sampleRate_ = 48000.0;
    SmoothedValue bassDb_, trebleDb_;
    float lastBass_ = 0.0f, lastTreble_ = 0.0f;
    Biquad bassL_, bassR_, trebleL_, trebleR_;
};

} // namespace veyra::dsp
