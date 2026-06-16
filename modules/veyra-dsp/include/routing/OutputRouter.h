#pragma once

// Sound Sharing router: turns a set of output routes (each with a per-output
// gain trim and a measured endpoint latency) into aligned, gain-trimmed stereo
// buffers — one per destination. The compensation delay for each route is
// (maxLatency - thisLatency), so every device emits the same audio at the same
// wall-clock instant. The WASAPI layer just feeds each route's endpoint with the
// buffer renderRoute() produces.

#include <algorithm>
#include <cmath>
#include <vector>

#include "enhancers/DelayLine.h"

namespace veyra::dsp {

struct RouteSpec {
    float gainDb = 0.0f;
    float latencyMs = 0.0f;
    bool  enabled = true;
};

class OutputRouter {
public:
    void prepare(double sampleRate, float maxDelayMs = 250.0f) noexcept
    {
        sampleRate_ = sampleRate;
        maxDelayMs_ = maxDelayMs;
    }

    void configure(const std::vector<RouteSpec>& specs)
    {
        const size_t n = specs.size();
        gains_.assign(n, 1.0f);
        enabled_.assign(n, false);
        delays_ = std::vector<DelayLine>(n);

        float maxLatency = 0.0f;
        for (const auto& s : specs)
            if (s.enabled)
                maxLatency = std::max(maxLatency, s.latencyMs);

        for (size_t i = 0; i < n; ++i)
        {
            enabled_[i] = specs[i].enabled;
            gains_[i] = std::pow(10.0f, specs[i].gainDb / 20.0f);
            delays_[i].prepare(sampleRate_, maxDelayMs_);
            // Slower (higher-latency) endpoints need less delay; the fastest
            // waits the longest so they all land together.
            const float comp = std::clamp(maxLatency - specs[i].latencyMs, 0.0f, maxDelayMs_);
            delays_[i].setDelayMs(comp);
        }
    }

    int routeCount() const noexcept { return (int) delays_.size(); }
    bool routeEnabled(int r) const noexcept { return r >= 0 && r < (int) enabled_.size() && enabled_[(size_t) r]; }
    int compensationSamples(int r) const noexcept { return delays_[(size_t) r].delaySamples(); }

    // Render destination 'r' from the shared stereo mix (in* may equal out*).
    void renderRoute(int r, const float* inL, const float* inR,
                     float* outL, float* outR, int numSamples) noexcept
    {
        if (r < 0 || r >= (int) delays_.size())
            return;
        if (!enabled_[(size_t) r])
        {
            for (int i = 0; i < numSamples; ++i) { outL[i] = 0.0f; outR[i] = 0.0f; }
            return;
        }
        const float g = gains_[(size_t) r];
        for (int i = 0; i < numSamples; ++i)
        {
            outL[i] = inL[i] * g;
            outR[i] = inR[i] * g;
        }
        delays_[(size_t) r].processStereo(outL, outR, numSamples);
    }

private:
    double sampleRate_ = 48000.0;
    float  maxDelayMs_ = 250.0f;
    std::vector<DelayLine> delays_;
    std::vector<float>     gains_;
    std::vector<char>      enabled_;
};

} // namespace veyra::dsp
