#pragma once

// Fixed-size radix-2 (Cooley-Tukey, decimation-in-time) real-input FFT used by
// the analyzer for the spectrum view. Tables (bit-reversal, twiddles, Hann
// window) are built once in prepare(); forwardMagnitude() allocates nothing,
// so it is safe in the real-time path.

#include <cmath>
#include <cstddef>
#include <vector>

namespace veyra::dsp {

class Fft {
public:
    void prepare(int order)
    {
        order_ = order;
        size_ = static_cast<size_t>(1) << order;

        re_.assign(size_, 0.0f);
        im_.assign(size_, 0.0f);
        rev_.resize(size_);
        window_.resize(size_);
        cos_.resize(size_ / 2);
        sin_.resize(size_ / 2);

        for (size_t i = 0; i < size_; ++i)
        {
            rev_[i] = bitReverse(i, order_);
            // Hann window to reduce spectral leakage.
            window_[i] = 0.5f * (1.0f - std::cos(2.0 * kPi * i / (size_ - 1)));
        }
        for (size_t k = 0; k < size_ / 2; ++k)
        {
            const double a = -2.0 * kPi * static_cast<double>(k) / static_cast<double>(size_);
            cos_[k] = static_cast<float>(std::cos(a));
            sin_[k] = static_cast<float>(std::sin(a));
        }
    }

    size_t size() const noexcept { return size_; }
    size_t numBins() const noexcept { return size_ / 2; }

    // 'time' must hold size() samples; 'mags' receives size()/2 magnitudes.
    void forwardMagnitude(const float* time, float* mags) noexcept
    {
        for (size_t i = 0; i < size_; ++i)
        {
            re_[i] = time[rev_[i]] * window_[rev_[i]];
            im_[i] = 0.0f;
        }

        for (int s = 1; s <= order_; ++s)
        {
            const size_t m = static_cast<size_t>(1) << s;
            const size_t m2 = m >> 1;
            const size_t step = size_ / m;
            for (size_t k = 0; k < size_; k += m)
            {
                for (size_t j = 0; j < m2; ++j)
                {
                    const size_t twi = j * step;
                    const float wr = cos_[twi];
                    const float wi = sin_[twi];
                    const size_t t = k + j;
                    const size_t u = t + m2;

                    const float vr = re_[u] * wr - im_[u] * wi;
                    const float vi = re_[u] * wi + im_[u] * wr;
                    const float ur = re_[t];
                    const float ui = im_[t];

                    re_[t] = ur + vr; im_[t] = ui + vi;
                    re_[u] = ur - vr; im_[u] = ui - vi;
                }
            }
        }

        for (size_t k = 0; k < size_ / 2; ++k)
            mags[k] = std::sqrt(re_[k] * re_[k] + im_[k] * im_[k]);
    }

private:
    static constexpr double kPi = 3.14159265358979323846;

    static size_t bitReverse(size_t v, int bits) noexcept
    {
        size_t r = 0;
        for (int i = 0; i < bits; ++i)
        {
            r = (r << 1) | (v & 1);
            v >>= 1;
        }
        return r;
    }

    int order_ = 0;
    size_t size_ = 0;
    std::vector<float> re_, im_, window_;
    std::vector<size_t> rev_;
    std::vector<float> cos_, sin_;
};

} // namespace veyra::dsp
