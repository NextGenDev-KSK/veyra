#pragma once

// HRTF headphone virtualisation. Treats the incoming L/R as two virtual
// loudspeakers at +/-30 degrees and renders each through the binaural HRTF
// (the same convolution path the rest of the spatial engine uses), summing to a
// stereo pair. The result is an "out-of-head", speaker-like image on headphones
// rather than the hard-panned in-head image stereo normally gives.
//
// It convolves in fixed blocks, so it buffers the (variable-size) call into its
// internal block and emits with one block of latency. Dry and wet are delayed
// equally, so the dry/wet mix stays phase-aligned. amount 0 = bit-exact dry.

#include <algorithm>
#include <filesystem>
#include <vector>

#include "spatial/Hrtf.h"
#include "spatial/HrtfDatabase.h"

namespace veyra::dsp {

class VirtualSurround {
public:
    // Point at the MIT KEMAR "diffuse" directory to use measured HRTFs (the
    // default in the shipped app); leave empty for the synthetic fallback. Call
    // before prepare().
    void setHrtfDirectory(std::filesystem::path dir) { hrtfDir_ = std::move(dir); }
    bool usingMeasured() const noexcept { return measured_; }

    void prepare(double sampleRate)
    {
        block_ = 256;

        // Prefer measured KEMAR IRs for the +/-30 virtual speakers; fall back to
        // the synthetic generator when the dataset isn't present.
        measured_ = false;
        if (!hrtfDir_.empty() && db_.load(hrtfDir_))
        {
            std::vector<float> lL, lR, rL, rR; // (left-ear, right-ear) per speaker
            if (db_.getHrir(-30.0f, 0.0f, sampleRate, lL, lR)
                && db_.getHrir(+30.0f, 0.0f, sampleRate, rL, rR)
                && !lL.empty() && !rL.empty())
            {
                panL_.prepareWithIr(block_, lL.data(), lR.data(), (int) lL.size());
                panR_.prepareWithIr(block_, rL.data(), rR.data(), (int) rL.size());
                measured_ = true;
            }
        }
        if (!measured_)
        {
            panL_.prepare(block_, sampleRate, -30.0f, 128); // left virtual speaker
            panR_.prepare(block_, sampleRate, +30.0f, 128); // right virtual speaker
        }

        const size_t n = (size_t) block_;
        inL_.assign(n, 0.0f);  inR_.assign(n, 0.0f);
        dryL_.assign(n, 0.0f); dryR_.assign(n, 0.0f);
        wetL_.assign(n, 0.0f); wetR_.assign(n, 0.0f);
        sLL_.assign(n, 0.0f);  sLR_.assign(n, 0.0f);
        sRL_.assign(n, 0.0f);  sRR_.assign(n, 0.0f);
        reset();
    }

    void reset()
    {
        std::fill(inL_.begin(), inL_.end(), 0.0f);   std::fill(inR_.begin(), inR_.end(), 0.0f);
        std::fill(dryL_.begin(), dryL_.end(), 0.0f); std::fill(dryR_.begin(), dryR_.end(), 0.0f);
        std::fill(wetL_.begin(), wetL_.end(), 0.0f); std::fill(wetR_.begin(), wetR_.end(), 0.0f);
        panL_.reset();
        panR_.reset();
        fill_ = 0;
    }

    void setAmount(float amount) noexcept { amount_ = std::clamp(amount, 0.0f, 1.0f); }
    float amount() const noexcept { return amount_; }

    void processStereo(float* left, float* right, int numSamples)
    {
        if (amount_ <= 0.0f || block_ == 0)
            return; // bit-exact dry

        for (int i = 0; i < numSamples; ++i)
        {
            const float curL = left[i], curR = right[i];
            inL_[(size_t) fill_] = curL;
            inR_[(size_t) fill_] = curR;

            // Emit the block-delayed dry/wet mix (aligned latency).
            const float w = amount_;
            left[i]  = (1.0f - w) * dryL_[(size_t) fill_] + w * wetL_[(size_t) fill_];
            right[i] = (1.0f - w) * dryR_[(size_t) fill_] + w * wetR_[(size_t) fill_];

            if (++fill_ >= block_)
            {
                processBlock();
                fill_ = 0;
            }
        }
    }

    int latencySamples() const noexcept { return block_; }

private:
    void processBlock()
    {
        panL_.process(inL_.data(), sLL_.data(), sLR_.data(), block_); // L speaker -> both ears
        panR_.process(inR_.data(), sRL_.data(), sRR_.data(), block_); // R speaker -> both ears
        for (int k = 0; k < block_; ++k)
        {
            wetL_[(size_t) k] = 0.5f * (sLL_[(size_t) k] + sRL_[(size_t) k]);
            wetR_[(size_t) k] = 0.5f * (sLR_[(size_t) k] + sRR_[(size_t) k]);
        }
        std::copy(inL_.begin(), inL_.end(), dryL_.begin());
        std::copy(inR_.begin(), inR_.end(), dryR_.begin());
    }

    int   block_ = 0;
    float amount_ = 0.0f;
    int   fill_ = 0;
    bool  measured_ = false;

    std::filesystem::path hrtfDir_;
    HrtfDatabase          db_;
    BinauralPanner panL_, panR_;
    std::vector<float> inL_, inR_, dryL_, dryR_, wetL_, wetR_;
    std::vector<float> sLL_, sLR_, sRL_, sRR_;
};

} // namespace veyra::dsp
