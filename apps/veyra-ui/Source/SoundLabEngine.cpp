#include "SoundLabEngine.h"

#include <cmath>

namespace veyra::ui {

void SoundLabEngine::start()
{
    if (started_)
        return;
    // 1 input (Mic Test) + 2 outputs; best-effort — output opens even with no mic.
    dm_.initialiseWithDefaultDevices(1, 2);
    dm_.addAudioCallback(this);
    started_ = true;
}

void SoundLabEngine::shutdown()
{
    if (!started_)
        return;
    dm_.removeAudioCallback(this);
    dm_.closeAudioDevice();
    started_ = false;
}

void SoundLabEngine::play(const veyra::dsp::LabSignalParams& p)
{
    start();
    const juce::SpinLock::ScopedLockType sl(lock_);
    params_  = p;
    playing_ = (p.type != veyra::dsp::TestSignal::Silence);
}

void SoundLabEngine::stop()
{
    const juce::SpinLock::ScopedLockType sl(lock_);
    playing_ = false;
    params_.type = veyra::dsp::TestSignal::Silence;
}

void SoundLabEngine::audioDeviceAboutToStart(juce::AudioIODevice* device)
{
    gen_.prepare(device->getCurrentSampleRate());
    scratch_.setSize(2, 8192, false, false, true);
    inputLevel_.store(0.0f);
}

void SoundLabEngine::audioDeviceStopped()
{
    inputLevel_.store(0.0f);
}

void SoundLabEngine::audioDeviceIOCallbackWithContext(const float* const* inputChannelData,
                                                      int numInputChannels,
                                                      float* const* outputChannelData,
                                                      int numOutputChannels,
                                                      int numSamples,
                                                      const juce::AudioIODeviceCallbackContext&)
{
    // Live input peak (Mic Test).
    float peak = 0.0f;
    if (numInputChannels > 0 && inputChannelData[0] != nullptr)
        for (int i = 0; i < numSamples; ++i)
            peak = juce::jmax(peak, std::abs(inputChannelData[0][i]));
    inputLevel_.store(peak, std::memory_order_relaxed);

    for (int c = 0; c < numOutputChannels; ++c)
        if (outputChannelData[c] != nullptr)
            juce::FloatVectorOperations::clear(outputChannelData[c], numSamples);

    bool play = false;
    veyra::dsp::LabSignalParams p;
    {
        const juce::SpinLock::ScopedTryLockType tl(lock_);
        if (tl.isLocked()) { play = playing_; p = params_; lastPlaying_ = play; lastParams_ = p; }
        else               { play = lastPlaying_; p = lastParams_; }
    }
    if (!play)
        return;

    const int n = juce::jmin(numSamples, scratch_.getNumSamples());
    gen_.setParams(p);
    float* L = scratch_.getWritePointer(0);
    float* R = scratch_.getWritePointer(1);
    gen_.renderStereo(L, R, n);

    if (numOutputChannels >= 2)
    {
        if (outputChannelData[0]) juce::FloatVectorOperations::copy(outputChannelData[0], L, n);
        if (outputChannelData[1]) juce::FloatVectorOperations::copy(outputChannelData[1], R, n);
    }
    else if (numOutputChannels == 1 && outputChannelData[0])
    {
        for (int i = 0; i < n; ++i) outputChannelData[0][i] = 0.5f * (L[i] + R[i]);
    }
}

} // namespace veyra::ui
