#pragma once

// True-peak meter (BS.1770 Annex 2 style): 4x oversamples each channel through a
// windowed-sinc polyphase interpolator and tracks the maximum reconstructed
// magnitude, so it catches inter-sample peaks a plain sample-peak meter misses.
// Reports linear true peak and dBTP. Allocation-free after prepare().

#include <array>
#include <cmath>

namespace veyra::dsp {

class TruePeakMeter {
public:
    void prepare(double /*sampleRate*/ = 48000.0)
    {
        buildKernel();
        reset();
    }

    void reset() noexcept
    {
        dlL_.fill(0.0f);
        dlR_.fill(0.0f);
        peak_ = 0.0f;
    }

    void processStereo(const float* left, const float* right, int numSamples) noexcept
    {
        for (int i = 0; i < numSamples; ++i)
        {
            push(dlL_, left[i]);
            push(dlR_, right[i]);
            for (int p = 0; p < kPhases; ++p)
            {
                peak_ = std::max(peak_, std::fabs(conv(dlL_, p)));
                peak_ = std::max(peak_, std::fabs(conv(dlR_, p)));
            }
        }
    }

    float truePeakLinear() const noexcept { return peak_; }
    float truePeakDb() const noexcept { return peak_ > 1.0e-9f ? 20.0f * std::log10(peak_) : -120.0f; }

private:
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

    void buildKernel()
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

    std::array<std::array<float, kTaps>, kPhases> kernel_{};
    std::array<float, kTaps> dlL_{}, dlR_{};
    float peak_ = 0.0f;
};

} // namespace veyra::dsp
