#pragma once

// EBU R128 / ITU-R BS.1770 loudness meter.
//
// Applies the two-stage K-weighting pre-filter (a high-shelf + an RLB high-pass,
// coefficients derived for any sample rate via the bilinear transform of the
// BS.1770 analog prototypes), accumulates mean square over 100 ms sub-blocks,
// and reports:
//   - momentary loudness  (400 ms window)
//   - short-term loudness (3 s window)
//   - integrated loudness (gated: -70 LUFS absolute + -10 LU relative gate)
// in LUFS. Allocation-free on the audio path (the integrated history grows per
// measurement session; call reset() to start a fresh measurement).
//
// Absolute calibration follows the standard (-0.691 dB offset + the K-weighting
// formulas); relative behaviour (level changes, frequency weighting, gating) is
// what the unit tests pin down.

#include <array>
#include <cmath>
#include <vector>

#include "eq/Biquad.h"

namespace veyra::dsp {

class LoudnessMeter {
public:
    void prepare(double sampleRate)
    {
        fs_ = sampleRate;
        computeKWeighting(sampleRate);
        subBlockLen_ = std::max(1, (int) (sampleRate * 0.1)); // 100 ms
        reset();
    }

    void reset()
    {
        shelfL_.reset(); shelfR_.reset(); hpL_.reset(); hpR_.reset();
        sumSq_ = 0.0;
        acc_ = 0;
        ringFill_ = 0;
        ringPos_ = 0;
        ring_.fill(0.0);
        blocks_.clear();
    }

    void processStereo(const float* left, const float* right, int numSamples)
    {
        for (int i = 0; i < numSamples; ++i)
        {
            const float l = hpL_.process(shelfL_.process(left[i]));
            const float r = hpR_.process(shelfR_.process(right[i]));
            sumSq_ += (double) l * l + (double) r * r; // L,R channel weight = 1.0

            if (++acc_ >= subBlockLen_)
            {
                pushSubBlock(sumSq_ / subBlockLen_);
                sumSq_ = 0.0;
                acc_ = 0;
            }
        }
    }

    // When false, the unbounded integrated history isn't accumulated (used by
    // the realtime normalizer, which only needs the bounded short-term window).
    void setIntegrating(bool on) noexcept { integrating_ = on; }

    float momentaryLufs() const { return windowLufs(4); }   // 400 ms
    float shortTermLufs() const { return windowLufs(30); }  // 3 s

    // Gated integrated loudness over the measurement session.
    float integratedLufs() const
    {
        if (blocks_.empty())
            return kSilence;

        // Absolute gate at -70 LUFS, then relative gate 10 LU below the
        // ungated (absolute-gated) mean.
        double sum = 0.0; int n = 0;
        for (double z : blocks_)
            if (loudnessOf(z) > -70.0) { sum += z; ++n; }
        if (n == 0)
            return kSilence;

        const double relGate = loudnessOf(sum / n) - 10.0;
        double gsum = 0.0; int gn = 0;
        for (double z : blocks_)
            if (loudnessOf(z) > relGate) { gsum += z; ++gn; }
        return gn ? (float) loudnessOf(gsum / gn) : kSilence;
    }

    static constexpr float kSilence = -70.0f; // floor reported for no/low signal

private:
    static double loudnessOf(double z) { return z > 0.0 ? -0.691 + 10.0 * std::log10(z) : -1000.0; }

    void pushSubBlock(double z)
    {
        ring_[(size_t) ringPos_] = z;
        ringPos_ = (ringPos_ + 1) % (int) ring_.size();
        if (ringFill_ < (int) ring_.size())
            ++ringFill_;

        // A 400 ms block = mean of the last 4 sub-blocks; feed the integrated set.
        if (integrating_ && ringFill_ >= 4)
            blocks_.push_back(windowMeanZ(4));
    }

    double windowMeanZ(int subBlocks) const
    {
        const int n = std::min(subBlocks, ringFill_);
        if (n <= 0)
            return 0.0;
        double s = 0.0;
        for (int k = 0; k < n; ++k)
        {
            const int idx = ((ringPos_ - 1 - k) % (int) ring_.size() + (int) ring_.size()) % (int) ring_.size();
            s += ring_[(size_t) idx];
        }
        return s / n;
    }

    float windowLufs(int subBlocks) const
    {
        if (ringFill_ <= 0)
            return kSilence;
        const double z = windowMeanZ(subBlocks);
        const double l = loudnessOf(z);
        return l < kSilence ? kSilence : (float) l;
    }

    void computeKWeighting(double fs)
    {
        constexpr double pi = 3.14159265358979323846;

        // Stage 1: high-shelf (+~4 dB).
        {
            const double f0 = 1681.974450955533, G = 3.999843853973347, Q = 0.7071752369554196;
            const double K = std::tan(pi * f0 / fs);
            const double Vh = std::pow(10.0, G / 20.0);
            const double Vb = std::pow(Vh, 0.4996667741545416);
            const double a0 = 1.0 + K / Q + K * K;
            BiquadCoeffs c;
            c.b0 = (float) ((Vh + Vb * K / Q + K * K) / a0);
            c.b1 = (float) (2.0 * (K * K - Vh) / a0);
            c.b2 = (float) ((Vh - Vb * K / Q + K * K) / a0);
            c.a1 = (float) (2.0 * (K * K - 1.0) / a0);
            c.a2 = (float) ((1.0 - K / Q + K * K) / a0);
            shelfL_.setCoeffs(c); shelfR_.setCoeffs(c);
        }
        // Stage 2: RLB high-pass (~38 Hz).
        {
            const double f0 = 38.13547087602444, Q = 0.5003270373238773;
            const double K = std::tan(pi * f0 / fs);
            const double a0 = 1.0 + K / Q + K * K;
            BiquadCoeffs c;
            c.b0 = (float) (1.0 / a0);
            c.b1 = (float) (-2.0 / a0);
            c.b2 = (float) (1.0 / a0);
            c.a1 = (float) (2.0 * (K * K - 1.0) / a0);
            c.a2 = (float) ((1.0 - K / Q + K * K) / a0);
            hpL_.setCoeffs(c); hpR_.setCoeffs(c);
        }
    }

    double fs_ = 48000.0;
    int    subBlockLen_ = 4800;
    Biquad shelfL_, shelfR_, hpL_, hpR_;

    double sumSq_ = 0.0;
    int    acc_ = 0;

    bool   integrating_ = true;
    std::array<double, 30> ring_{}; // last 30 sub-blocks (3 s) of mean-square
    int    ringFill_ = 0, ringPos_ = 0;

    std::vector<double> blocks_; // 400 ms block mean-squares for integrated gating
};

} // namespace veyra::dsp
