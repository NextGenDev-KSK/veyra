#pragma once

// Post-chain metering for the UI: per-channel RMS + peak (VU), a clipping flag,
// and a windowed FFT magnitude spectrum. Frames are produced once every
// kFftSize samples (~10.7 ms at 48 kHz, ~93 fps) and pushed to a lock-free ring
// buffer that the UI drains at its leisure. process() allocates nothing.

#include <algorithm>
#include <cmath>

#include "analyzer/Fft.h"
#include "analyzer/SpscRingBuffer.h"

namespace veyra::dsp {

inline constexpr int kAnalyzerFftOrder = 9;             // 512-point
inline constexpr int kAnalyzerFftSize = 1 << kAnalyzerFftOrder;
inline constexpr int kAnalyzerBins = kAnalyzerFftSize / 2; // 256

struct AnalyzerFrame {
    float rmsL = 0.0f, rmsR = 0.0f;
    float peakL = 0.0f, peakR = 0.0f;
    bool  clipped = false;
    float spectrum[kAnalyzerBins] = {};
};

class Analyzer {
public:
    void prepare(double sampleRate)
    {
        sampleRate_ = sampleRate;
        fft_.prepare(kAnalyzerFftOrder);
        writePos_ = 0;
        resetAccumulators();
    }

    void processStereo(const float* left, const float* right, int numSamples) noexcept
    {
        for (int i = 0; i < numSamples; ++i)
        {
            const float l = left[i];
            const float r = right[i];

            sumSqL_ += l * l;
            sumSqR_ += r * r;
            peakL_ = std::max(peakL_, std::fabs(l));
            peakR_ = std::max(peakR_, std::fabs(r));
            if (std::fabs(l) >= 1.0f || std::fabs(r) >= 1.0f)
                clipped_ = true;
            ++count_;

            fftInput_[writePos_++] = 0.5f * (l + r);
            if (writePos_ == kAnalyzerFftSize)
                emitFrame();
        }
    }

    // Consumer (UI thread): drain the latest available frame(s).
    bool popFrame(AnalyzerFrame& out) noexcept { return ring_.pop(out); }

private:
    void resetAccumulators() noexcept
    {
        sumSqL_ = sumSqR_ = 0.0;
        peakL_ = peakR_ = 0.0f;
        clipped_ = false;
        count_ = 0;
    }

    void emitFrame() noexcept
    {
        AnalyzerFrame f;
        const double n = count_ > 0 ? static_cast<double>(count_) : 1.0;
        f.rmsL = static_cast<float>(std::sqrt(sumSqL_ / n));
        f.rmsR = static_cast<float>(std::sqrt(sumSqR_ / n));
        f.peakL = peakL_;
        f.peakR = peakR_;
        f.clipped = clipped_;
        fft_.forwardMagnitude(fftInput_, f.spectrum);

        ring_.push(f); // dropped silently if the UI hasn't drained in time

        writePos_ = 0;
        resetAccumulators();
    }

    double sampleRate_ = 48000.0;
    Fft fft_;
    float fftInput_[kAnalyzerFftSize] = {};
    int writePos_ = 0;

    double sumSqL_ = 0.0, sumSqR_ = 0.0;
    float peakL_ = 0.0f, peakR_ = 0.0f;
    bool clipped_ = false;
    int count_ = 0;

    SpscRingBuffer<AnalyzerFrame, 16> ring_;
};

} // namespace veyra::dsp
