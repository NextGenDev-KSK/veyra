#include "veyra/RnnoiseDenoiser.h"

#if VEYRA_USE_RNNOISE
#include <cstring>
#include <rnnoise.h>
#endif

namespace veyra {

struct RnnoiseDenoiser::Impl {
#if VEYRA_USE_RNNOISE
    static constexpr int kFrame  = 480;  // RNNoise frame @ 48 kHz
    static constexpr int kOutCap = 4096; // output ring (>= kFrame + max block)

    DenoiseState* st = nullptr;
    bool  active = false;
    float inFrame[kFrame] = {0};
    int   inFill = 0;
    float tmp[kFrame] = {0};
    float outBuf[kOutCap] = {0};
    int   outRead = 0, outWrite = 0, outAvail = 0;
#endif
};

RnnoiseDenoiser::RnnoiseDenoiser() : impl_(new Impl()) {}

RnnoiseDenoiser::~RnnoiseDenoiser()
{
#if VEYRA_USE_RNNOISE
    if (impl_->st) rnnoise_destroy(impl_->st);
#endif
    delete impl_;
}

void RnnoiseDenoiser::prepare(double sampleRate)
{
#if VEYRA_USE_RNNOISE
    if (impl_->st) { rnnoise_destroy(impl_->st); impl_->st = nullptr; }
    impl_->active = (sampleRate == 48000.0); // RNNoise is 48 kHz only
    if (impl_->active)
        impl_->st = rnnoise_create(nullptr); // built-in model
    reset();
#else
    (void) sampleRate;
#endif
}

void RnnoiseDenoiser::reset()
{
#if VEYRA_USE_RNNOISE
    impl_->inFill = 0;
    impl_->outRead = impl_->outWrite = impl_->outAvail = 0;
    std::memset(impl_->inFrame, 0, sizeof impl_->inFrame);
    std::memset(impl_->outBuf, 0, sizeof impl_->outBuf);
#endif
}

bool RnnoiseDenoiser::active() const noexcept
{
#if VEYRA_USE_RNNOISE
    return impl_->active && impl_->st != nullptr;
#else
    return false;
#endif
}

void RnnoiseDenoiser::processMono(float* x, int numSamples) noexcept
{
#if VEYRA_USE_RNNOISE
    if (!active())
        return; // pass-through
    Impl& I = *impl_;
    for (int i = 0; i < numSamples; ++i)
    {
        I.inFrame[I.inFill++] = x[i] * 32768.0f; // RNNoise works in int16 scale

        if (I.outAvail > 0)
        {
            x[i] = I.outBuf[I.outRead] / 32768.0f;
            I.outRead = (I.outRead + 1) % Impl::kOutCap;
            --I.outAvail;
        }
        else
        {
            x[i] = 0.0f; // priming latency (< one 480-frame)
        }

        if (I.inFill == Impl::kFrame)
        {
            rnnoise_process_frame(I.st, I.tmp, I.inFrame);
            for (int k = 0; k < Impl::kFrame; ++k)
            {
                if (I.outAvail >= Impl::kOutCap) break; // guard (shouldn't happen)
                I.outBuf[I.outWrite] = I.tmp[k];
                I.outWrite = (I.outWrite + 1) % Impl::kOutCap;
                ++I.outAvail;
            }
            I.inFill = 0;
        }
    }
#else
    (void) x; (void) numSamples;
#endif
}

} // namespace veyra
