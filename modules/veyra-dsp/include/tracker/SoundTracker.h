// The Sound Tracker (Gamer Mode): listens to the post-mix stereo stream, and
// every ~10 ms frame extracts features, classifies the dominant sound, and
// estimates its direction. Actionable detections (footstep / gunshot / voice)
// are pushed to a lock-free queue the overlay drains to draw the radar.
//
// processStereo() is allocation-free (buffers sized in prepare()); the producer
// is the audio/analysis thread, the consumer is the overlay/UI thread.
#pragma once

#include <cmath>
#include <vector>

#include "analyzer/SpscRingBuffer.h"
#include "tracker/DirectionEstimator.h"
#include "tracker/FeatureExtractor.h"
#include "tracker/SoundClassifier.h"
#include "tracker/TrackerTypes.h"

namespace veyra::dsp {

class SoundTracker {
public:
    void prepare(double sampleRate)
    {
        sampleRate_ = sampleRate;
        frameSize_ = std::max(64, static_cast<int>(sampleRate * 0.010)); // ~10 ms
        mono_.assign(static_cast<size_t>(frameSize_), 0.0f);
        features_.prepare(sampleRate);
        reset();
    }

    void reset() noexcept
    {
        features_.reset();
        pos_ = 0;
        sumSqL_ = sumSqR_ = 0.0;
    }

    void setSensitivity(float s) noexcept { classifier_.setSensitivity(s); }

    void processStereo(const float* left, const float* right, int numSamples) noexcept
    {
        for (int i = 0; i < numSamples; ++i)
        {
            const float l = left[i], r = right[i];
            sumSqL_ += (double) l * l;
            sumSqR_ += (double) r * r;
            mono_[(size_t) pos_] = 0.5f * (l + r);
            if (++pos_ >= frameSize_)
            {
                emitFrame();
                pos_ = 0;
                sumSqL_ = sumSqR_ = 0.0;
            }
        }
    }

    bool popEvent(TrackerEvent& out) noexcept { return queue_.pop(out); }

private:
    void emitFrame() noexcept
    {
        const AudioFeatures f = features_.process(mono_.data(), frameSize_);
        const float inv = 1.0f / (float) frameSize_;
        const float rmsL = std::sqrt((float) sumSqL_ * inv);
        const float rmsR = std::sqrt((float) sumSqR_ * inv);
        const auto dir = DirectionEstimator::fromStereoRms(rmsL, rmsR);
        const SoundClass c = classifier_.classify(f);

        if (c == SoundClass::Footstep || c == SoundClass::Gunshot || c == SoundClass::Voice)
        {
            TrackerEvent ev;
            ev.type = c;
            ev.azimuthDeg = dir.azimuthDeg;
            ev.intensity = f.rms;
            ev.confidence = std::min(1.0f, dir.energy);
            queue_.push(ev); // drops silently if the consumer is behind
        }
    }

    double sampleRate_ = 48000.0;
    int    frameSize_ = 480;
    int    pos_ = 0;
    double sumSqL_ = 0.0, sumSqR_ = 0.0;

    std::vector<float> mono_;
    FeatureExtractor   features_;
    SoundClassifier    classifier_;
    SpscRingBuffer<TrackerEvent, 64> queue_;
};

} // namespace veyra::dsp
