#pragma once

// The no-driver processing path. When enabled, the bridge WASAPI-loopback-
// captures the source endpoint (set it as the Windows default so apps play into
// it — typically a virtual sink), runs the shared veyra::dsp::DspChain with the
// live config, and renders the processed audio to the target endpoint (the real
// headphones). This is how Veyra shapes Spotify/YouTube audio without the APO,
// which matters for Bluetooth outputs that reject endpoint APOs.
//
// v1 limitations (logged at runtime): source and target must share sample rate
// and be stereo 32-bit float (shared-mode default); no resampling yet.

#include <atomic>
#include <mutex>
#include <thread>

#include "veyra/Config.h"

namespace veyra {
class Logger;
}

namespace veyra::service {

class AudioBridge {
public:
    explicit AudioBridge(Logger* log = nullptr) : log_(log) {}
    ~AudioBridge();

    bool start();
    void stop();

    // Apply config (enabled, device ids, and the live DSP params). Thread-safe.
    void setConfig(const Config& config);

private:
    void run();
    bool session(); // one capture/render session; false on error (caller backs off)

    Logger*           log_;
    std::thread       thread_;
    std::atomic<bool> running_{false};

    std::mutex mutex_;
    Config     cfg_;
    bool       haveCfg_ = false;
};

} // namespace veyra::service
