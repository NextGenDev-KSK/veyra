#pragma once

// Uniformly-partitioned overlap-save FFT convolution (UPOLS). The impulse
// response is split into fixed-size partitions, each transformed once; every
// process block transforms one input frame and accumulates the per-partition
// products through a frequency-domain delay line. This gives constant low
// latency (one block) regardless of IR length — the engine behind HRTF
// virtualisation and reverb.
//
// process() is called with exactly blockSize() samples. Allocation happens in
// prepare(); process() is allocation-free.

#include <cmath>
#include <cstddef>
#include <vector>

namespace veyra::dsp {

namespace detail {

// Minimal in-place iterative radix-2 complex FFT (forward + inverse).
class ComplexFft {
public:
    void prepare(int order)
    {
        order_ = order;
        n_ = static_cast<size_t>(1) << order;
        rev_.resize(n_);
        cos_.resize(n_ / 2);
        sin_.resize(n_ / 2);
        for (size_t i = 0; i < n_; ++i)
            rev_[i] = bitReverse(i, order_);
        for (size_t k = 0; k < n_ / 2; ++k)
        {
            const double a = -2.0 * kPi * static_cast<double>(k) / static_cast<double>(n_);
            cos_[k] = static_cast<float>(std::cos(a));
            sin_[k] = static_cast<float>(std::sin(a));
        }
    }

    size_t size() const noexcept { return n_; }

    // In-place transform. 'inverse' conjugates the twiddles and scales by 1/N.
    void transform(float* re, float* im, bool inverse) const
    {
        for (size_t i = 0; i < n_; ++i)
        {
            const size_t j = rev_[i];
            if (j > i)
            {
                std::swap(re[i], re[j]);
                std::swap(im[i], im[j]);
            }
        }

        for (int s = 1; s <= order_; ++s)
        {
            const size_t m = static_cast<size_t>(1) << s;
            const size_t m2 = m >> 1;
            const size_t step = n_ / m;
            for (size_t k = 0; k < n_; k += m)
            {
                for (size_t j = 0; j < m2; ++j)
                {
                    const size_t twi = j * step;
                    const float wr = cos_[twi];
                    const float wi = inverse ? -sin_[twi] : sin_[twi];
                    const size_t t = k + j;
                    const size_t u = t + m2;

                    const float vr = re[u] * wr - im[u] * wi;
                    const float vi = re[u] * wi + im[u] * wr;
                    re[u] = re[t] - vr; im[u] = im[t] - vi;
                    re[t] += vr;        im[t] += vi;
                }
            }
        }

        if (inverse)
        {
            const float inv = 1.0f / static_cast<float>(n_);
            for (size_t i = 0; i < n_; ++i) { re[i] *= inv; im[i] *= inv; }
        }
    }

private:
    static constexpr double kPi = 3.14159265358979323846;
    static size_t bitReverse(size_t v, int bits) noexcept
    {
        size_t r = 0;
        for (int i = 0; i < bits; ++i) { r = (r << 1) | (v & 1); v >>= 1; }
        return r;
    }

    int order_ = 0;
    size_t n_ = 0;
    std::vector<size_t> rev_;
    std::vector<float> cos_, sin_;
};

inline int nextPow2Order(size_t v)
{
    int o = 0;
    size_t n = 1;
    while (n < v) { n <<= 1; ++o; }
    return o;
}

} // namespace detail

class PartitionedConvolver {
public:
    void prepare(int blockSize, const float* ir, int irLen)
    {
        B_ = blockSize;
        const int order = detail::nextPow2Order(static_cast<size_t>(2 * B_));
        N_ = static_cast<size_t>(1) << order;
        fft_.prepare(order);

        parts_ = irLen > 0 ? (irLen + B_ - 1) / B_ : 1;

        Hre_.assign(static_cast<size_t>(parts_), std::vector<float>(N_, 0.0f));
        Him_.assign(static_cast<size_t>(parts_), std::vector<float>(N_, 0.0f));
        for (int p = 0; p < parts_; ++p)
        {
            for (int i = 0; i < B_; ++i)
            {
                const int idx = p * B_ + i;
                Hre_[(size_t) p][(size_t) i] = (idx < irLen) ? ir[idx] : 0.0f;
            }
            fft_.transform(Hre_[(size_t) p].data(), Him_[(size_t) p].data(), false);
        }

        Xre_.assign(static_cast<size_t>(parts_), std::vector<float>(N_, 0.0f));
        Xim_.assign(static_cast<size_t>(parts_), std::vector<float>(N_, 0.0f));
        fdlPos_ = 0;

        inBuf_.assign(N_, 0.0f);
        yRe_.assign(N_, 0.0f);
        yIm_.assign(N_, 0.0f);
        ready_ = true;
        reset();
    }

    void reset()
    {
        std::fill(inBuf_.begin(), inBuf_.end(), 0.0f);
        for (auto& v : Xre_) std::fill(v.begin(), v.end(), 0.0f);
        for (auto& v : Xim_) std::fill(v.begin(), v.end(), 0.0f);
        fdlPos_ = 0;
    }

    int blockSize() const noexcept { return B_; }

    // 'in' and 'out' hold blockSize() samples (may alias).
    void process(const float* in, float* out, int numSamples)
    {
        if (!ready_ || numSamples != B_)
        {
            if (in != out)
                for (int i = 0; i < numSamples; ++i) out[i] = in[i];
            return;
        }

        // Slide the 2B input frame: [previous B | current B].
        for (int i = 0; i < B_; ++i)
            inBuf_[(size_t) i] = inBuf_[(size_t) (i + B_)];
        for (int i = 0; i < B_; ++i)
            inBuf_[(size_t) (B_ + i)] = in[i];

        // Forward-transform the current frame into the FDL slot.
        auto& Xr = Xre_[(size_t) fdlPos_];
        auto& Xi = Xim_[(size_t) fdlPos_];
        for (size_t i = 0; i < N_; ++i) { Xr[i] = inBuf_[i]; Xi[i] = 0.0f; }
        fft_.transform(Xr.data(), Xi.data(), false);

        // Accumulate H_p * X_{n-p} across partitions.
        std::fill(yRe_.begin(), yRe_.end(), 0.0f);
        std::fill(yIm_.begin(), yIm_.end(), 0.0f);
        for (int p = 0; p < parts_; ++p)
        {
            const int slot = ((fdlPos_ - p) % parts_ + parts_) % parts_;
            const auto& hr = Hre_[(size_t) p];
            const auto& hi = Him_[(size_t) p];
            const auto& xr = Xre_[(size_t) slot];
            const auto& xi = Xim_[(size_t) slot];
            for (size_t k = 0; k < N_; ++k)
            {
                yRe_[k] += hr[k] * xr[k] - hi[k] * xi[k];
                yIm_[k] += hr[k] * xi[k] + hi[k] * xr[k];
            }
        }

        fft_.transform(yRe_.data(), yIm_.data(), true); // inverse

        // Overlap-save: the valid output is the last B samples.
        for (int i = 0; i < B_; ++i)
            out[i] = yRe_[(size_t) (B_ + i)];

        fdlPos_ = (fdlPos_ + 1) % parts_;
    }

private:
    detail::ComplexFft fft_;
    int B_ = 0;
    size_t N_ = 0;
    int parts_ = 0;
    int fdlPos_ = 0;
    bool ready_ = false;

    std::vector<std::vector<float>> Hre_, Him_; // IR partitions (frequency)
    std::vector<std::vector<float>> Xre_, Xim_; // input FDL (frequency)
    std::vector<float> inBuf_, yRe_, yIm_;
};

} // namespace veyra::dsp
