#pragma once

// HRTF binaural panning. A mono source at a given azimuth is rendered to a
// stereo (headphone) pair by convolving it with a left/right head-related
// impulse response.
//
// makeSyntheticHrir() generates a lightweight physically-motivated HRIR pair
// (inter-aural time difference + level difference + a head-shadow low-pass on
// the far ear). It is the default so the engine works with no dataset; the real
// MIT KEMAR measurements drop in through the same (left, right) IR arrays once
// the dataset is present (see third_party/hrtf/mit_kemar). BinauralPanner then
// runs the pair through partitioned convolution — the same path for synthetic
// and measured IRs.

#include <cmath>
#include <vector>

#include "spatial/Convolver.h"

namespace veyra::dsp {

// azimuthDeg: 0 = front, negative = left, positive = right.
inline void makeSyntheticHrir(float azimuthDeg, double sampleRate, int irLen,
                              std::vector<float>& left, std::vector<float>& right)
{
    left.assign(static_cast<size_t>(irLen), 0.0f);
    right.assign(static_cast<size_t>(irLen), 0.0f);

    const double az = azimuthDeg * 3.14159265358979323846 / 180.0;
    const float  s = static_cast<float>(std::sin(az));         // -1 (left) .. +1 (right)
    const int    itd = static_cast<int>(std::lround(0.0007 * sampleRate * std::fabs(s)));
    const float  gFar = 1.0f - 0.35f * std::fabs(s);           // inter-aural level diff

    auto writeNear = [&](std::vector<float>& ir)
    {
        if (irLen > 0)
            ir[0] = 1.0f; // crisp near-ear impulse
    };
    auto writeFar = [&](std::vector<float>& ir)
    {
        // A short one-pole-smoothed click at the ITD delay = head-shadow low-pass.
        const float a = 0.5f;
        float v = gFar * (1.0f - a);
        for (int k = itd; k < irLen && v > 1.0e-4f; ++k)
        {
            ir[(size_t) k] += v;
            v *= a;
        }
    };

    if (s >= 0.0f) { writeNear(right); writeFar(left); }  // source to the right
    else           { writeNear(left);  writeFar(right); } // source to the left
}

class BinauralPanner {
public:
    void prepare(int blockSize, double sampleRate, float azimuthDeg, int irLen = 128)
    {
        std::vector<float> l, r;
        makeSyntheticHrir(azimuthDeg, sampleRate, irLen, l, r);
        convL_.prepare(blockSize, l.data(), irLen);
        convR_.prepare(blockSize, r.data(), irLen);
    }

    void reset()
    {
        convL_.reset();
        convR_.reset();
    }

    // 'mono' holds blockSize() samples; outL/outR receive the binaural pair.
    void process(const float* mono, float* outL, float* outR, int numSamples)
    {
        convL_.process(mono, outL, numSamples);
        convR_.process(mono, outR, numSamples);
    }

    int blockSize() const noexcept { return convL_.blockSize(); }

private:
    PartitionedConvolver convL_, convR_;
};

} // namespace veyra::dsp
