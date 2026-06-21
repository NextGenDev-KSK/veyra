#pragma once

// RNNoise mic denoiser wrapper. Opaque (no rnnoise types in the header) so it
// can sit in the capture APO. Aggregates the stream into RNNoise's fixed 480-
// sample / 48 kHz frames and emits with bounded latency; processMono() is
// allocation-free (RT-safe). When RNNoise isn't compiled in, or the rate isn't
// 48 kHz, active() is false and processMono() is a pass-through — the chain's
// own suppressor stays the fallback.

namespace veyra {

class RnnoiseDenoiser {
public:
    RnnoiseDenoiser();
    ~RnnoiseDenoiser();
    RnnoiseDenoiser(const RnnoiseDenoiser&) = delete;
    RnnoiseDenoiser& operator=(const RnnoiseDenoiser&) = delete;

    void prepare(double sampleRate);
    void reset();
    bool active() const noexcept; // RNNoise compiled in AND sampleRate == 48 kHz
    void processMono(float* x, int numSamples) noexcept;

private:
    struct Impl;
    Impl* impl_;
};

} // namespace veyra
