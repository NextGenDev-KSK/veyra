#pragma once

// Sound Lab test-signal engine: opens the default audio device and renders the
// reusable veyra::dsp::SignalGenerator (sine / sweep / white / pink, per-channel,
// polarity invert) to the output, and exposes the live input peak for the Mic
// Test. Runtime/compile-verified (real audio needs a device).

#include "VeyraGui.h"

#include "lab/SignalGenerator.h"

#include <atomic>

namespace veyra::ui {

class SoundLabEngine : public juce::AudioIODeviceCallback {
public:
    SoundLabEngine() = default;
    ~SoundLabEngine() override { shutdown(); }

    void start();                                       // open default device (idempotent)
    void shutdown();
    void play(const veyra::dsp::LabSignalParams& p);    // start/replace the signal
    void stop();                                        // silence (device stays open)

    float inputLevel() const noexcept { return inputLevel_.load(std::memory_order_relaxed); }

    // juce::AudioIODeviceCallback
    void audioDeviceIOCallbackWithContext(const float* const* inputChannelData,
                                          int numInputChannels,
                                          float* const* outputChannelData,
                                          int numOutputChannels,
                                          int numSamples,
                                          const juce::AudioIODeviceCallbackContext& context) override;
    void audioDeviceAboutToStart(juce::AudioIODevice* device) override;
    void audioDeviceStopped() override;

private:
    juce::AudioDeviceManager     dm_;
    veyra::dsp::SignalGenerator  gen_;
    juce::AudioBuffer<float>     scratch_;
    juce::SpinLock               lock_;
    veyra::dsp::LabSignalParams  params_;          // guarded by lock_
    bool                         playing_ = false; // guarded by lock_
    veyra::dsp::LabSignalParams  lastParams_;      // callback fallback if try-lock fails
    bool                         lastPlaying_ = false;
    bool                         started_ = false;
    std::atomic<float>           inputLevel_{0.0f};
};

} // namespace veyra::ui
