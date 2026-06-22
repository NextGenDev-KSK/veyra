#pragma once

// Lookahead brick-wall limiter with a hard mathematical guarantee that the
// output magnitude never exceeds the ceiling.
//
// The trick: delay the signal by the lookahead, and drive the gain from a
// sliding-window maximum of the link peak over the same window. The sample
// being output is the *oldest* in that window, so windowMax >= |out|, and
//   |out * (ceiling / windowMax)| <= ceiling
// always holds. Release is one-poled but clamped to never rise above that safe
// gain, so the guarantee survives smoothing.
//
// True-peak: the detector feeds max(|sample|, inter-sample peak) into the
// window, where the inter-sample peak comes from a 4x windowed-sinc polyphase
// reconstruction (BS.1770-style, same kernel as TruePeakMeter). Since the
// |sample| term is kept, the exact sample-peak guarantee above is preserved
// untouched; the oversampled term only ever raises the detected peak, so the
// limiter additionally tames inter-sample overshoots. No extra latency: the
// wide 65-sample window absorbs the interpolator's few-sample group delay.

#include <algorithm>
#include <array>
#include <cmath>

namespace veyra::dsp {

class Limiter {
public:
    static constexpr int kLookahead = 64;          // ~1.3 ms at 48 kHz
    static constexpr int kWindow = kLookahead + 1; // window includes the output sample

    void prepare(double sampleRate) noexcept
    {
        sampleRate_ = sampleRate;
        buildKernel();
        setReleaseMs(releaseMs_);
        reset();
    }

    void reset() noexcept
    {
        bufL_.fill(0.0f);
        bufR_.fill(0.0f);
        bufPeak_.fill(0.0f);
        dlL_.fill(0.0f);
        dlR_.fill(0.0f);
        pos_ = 0;
        gain_ = 1.0f;
    }

    void setCeilingDb(float db) noexcept { ceiling_ = std::pow(10.0f, db / 20.0f); }
    void setReleaseMs(float ms) noexcept
    {
        releaseMs_ = ms;
        const double t = (ms * 0.001) * sampleRate_;
        releaseCoeff_ = t > 0.0 ? static_cast<float>(std::exp(-1.0 / t)) : 0.0f;
    }

    int latencySamples() const noexcept { return kLookahead; }

    void processStereo(float* left, float* right, int numSamples) noexcept
    {
        for (int i = 0; i < numSamples; ++i)
        {
            const float peak = std::max(std::fabs(left[i]), std::fabs(right[i]));

            // Inter-sample (true) peak: 4x-oversample the reconstruction and take
            // the largest reconstructed magnitude. max() with the raw sample peak
            // keeps the exact sample-peak guarantee intact.
            push(dlL_, left[i]);
            push(dlR_, right[i]);
            float isp = peak;
            for (int p = 0; p < kPhases; ++p)
            {
                isp = std::max(isp, std::fabs(conv(dlL_, p)));
                isp = std::max(isp, std::fabs(conv(dlR_, p)));
            }

            // Write current sample, then advance to the oldest slot.
            bufL_[pos_] = left[i];
            bufR_[pos_] = right[i];
            bufPeak_[pos_] = isp;
            pos_ = (pos_ + 1) % kWindow;

            const float outL = bufL_[pos_];
            const float outR = bufR_[pos_];

            float windowMax = 0.0f;
            for (int k = 0; k < kWindow; ++k)
                windowMax = std::max(windowMax, bufPeak_[k]);

            const float safeGain = windowMax > ceiling_ ? ceiling_ / windowMax : 1.0f;

            // Instant attack (more reduction); clamped release (never above safe).
            if (safeGain < gain_)
                gain_ = safeGain;
            else
                gain_ = safeGain + releaseCoeff_ * (gain_ - safeGain);

            left[i]  = outL * gain_;
            right[i] = outR * gain_;
        }
    }

private:
    // --- 4x windowed-sinc polyphase interpolator (inter-sample peak) ---------
    static constexpr int kPhases = 4;
    static constexpr int kTaps = 8; // taps per phase

    void push(std::array<float, kTaps>& dl, float x) noexcept
    {
        for (int k = 0; k < kTaps - 1; ++k)
            dl[(size_t) k] = dl[(size_t) (k + 1)];
        dl[(size_t) (kTaps - 1)] = x;
    }

    float conv(const std::array<float, kTaps>& dl, int phase) const noexcept
    {
        float acc = 0.0f;
        for (int k = 0; k < kTaps; ++k)
            acc += kernel_[(size_t) phase][(size_t) k] * dl[(size_t) (kTaps - 1 - k)];
        return acc;
    }

    void buildKernel() noexcept
    {
        constexpr double pi = 3.14159265358979323846;
        constexpr int L = kPhases * kTaps;
        const double center = (L - 1) / 2.0;

        std::array<double, L> proto{};
        for (int n = 0; n < L; ++n)
        {
            const double x = (n - center) / kPhases; // sinc cutoff at fs/2
            const double s = (std::fabs(x) < 1.0e-9) ? 1.0 : std::sin(pi * x) / (pi * x);
            const double w = 0.5 - 0.5 * std::cos(2.0 * pi * n / (L - 1)); // Hann
            proto[(size_t) n] = s * w;
        }
        for (int p = 0; p < kPhases; ++p)
        {
            double sum = 0.0;
            for (int k = 0; k < kTaps; ++k)
            {
                kernel_[(size_t) p][(size_t) k] = (float) proto[(size_t) (k * kPhases + p)];
                sum += kernel_[(size_t) p][(size_t) k];
            }
            if (sum != 0.0) // unity DC gain per phase
                for (int k = 0; k < kTaps; ++k)
                    kernel_[(size_t) p][(size_t) k] /= (float) sum;
        }
    }

    double sampleRate_ = 48000.0;
    float ceiling_ = 0.966f; // ~ -0.3 dBFS
    float releaseMs_ = 50.0f;
    float releaseCoeff_ = 0.0f;

    std::array<float, kWindow> bufL_{};
    std::array<float, kWindow> bufR_{};
    std::array<float, kWindow> bufPeak_{};
    int pos_ = 0;
    float gain_ = 1.0f;

    std::array<std::array<float, kTaps>, kPhases> kernel_{};
    std::array<float, kTaps> dlL_{}, dlR_{};
};

} // namespace veyra::dsp
