#pragma once

// The audio path on unsigned builds (and the only path for Bluetooth): WASAPI-
// loopback-capture a source endpoint (the virtual sink apps play into), run the
// shared veyra::dsp::DspChain with the live config, and render the processed
// audio to the target endpoint. On signed builds the APO path takes over for
// wired endpoints and the bridge remains the Bluetooth route.
//
// Latency: the render side is event-driven; when the source rate matches the
// target's mix format the stream opens through IAudioClient3 at the engine's
// minimum period (typically ~3 ms buffers), otherwise a 20 ms event-driven
// stream with Windows auto-conversion. The session thread joins the Pro Audio
// MMCSS class.
//
// Sources: 32-bit float shared-mode mix in any common layout; mono is upmixed
// and multichannel (5.1 / 7.1) is downmixed to stereo before the chain.

#include <atomic>
#include <mutex>
#include <thread>

#include "veyra/Config.h"

// Forward-declare the Win32 handle so the header stays lean.
using VeyraWinHandle = void*;

namespace veyra {
class Logger;
}

namespace veyra::service {

class AudioBridge {
public:
    explicit AudioBridge(Logger* log = nullptr);
    ~AudioBridge();

    bool start();
    void stop();

    // Apply config (enabled, device ids, and the live DSP params). Thread-safe.
    void setConfig(const Config& config);

    // Wake the worker immediately (device arrival/removal, power resume) instead
    // of waiting out the idle backoff. Safe from any thread, including COM
    // notification callbacks.
    void kick();

private:
    void run();
    bool session(); // one capture/render session; false on error (caller backs off)

    Logger*           log_;
    std::thread       thread_;
    std::atomic<bool> running_{false};
    VeyraWinHandle    kickEvent_ = nullptr; // auto-reset; breaks the idle backoff

    std::mutex mutex_;
    Config     cfg_;
    bool       haveCfg_ = false;
};

} // namespace veyra::service
