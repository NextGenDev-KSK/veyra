#pragma once

// Advanced compatibility path for audio endpoints that reject the APO (primarily
// Bluetooth). The APO path (veyra-apo.dll into audiodg.exe) is the primary
// architecture; use the bridge only when the target endpoint cannot host an APO.
//
// When enabled, the bridge WASAPI-loopback-captures a source endpoint, runs the
// shared veyra::dsp::DspChain with the live config, and renders the processed
// audio to the target endpoint (e.g. Bluetooth headphones).
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
