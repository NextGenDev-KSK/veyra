#pragma once

// Microphone processing without a signed APO: capture the microphone, run the
// same veyra::dsp::VoiceChain the capture APO uses (with RNNoise when the rate
// allows), and render the cleaned voice into a virtual cable's render endpoint.
// Apps then select the cable's capture side (e.g. "CABLE Output") as their
// microphone and receive the processed signal.
//
//   real mic -> VoiceChain (RNNoise / gate / AGC / de-ess / presence) -> cable
//
// Needs its own virtual cable: if the target is the same endpoint the playback
// AudioBridge captures, apps' audio would mix into the mic, so the session
// refuses and idles (the Devices card explains this in the UI).

#include <atomic>
#include <mutex>
#include <thread>

#include "veyra/Config.h"

using VeyraWinHandle = void*;

namespace veyra {
class Logger;
}

namespace veyra::service {

class MicBridge {
public:
    explicit MicBridge(Logger* log = nullptr);
    ~MicBridge();

    bool start();
    void stop();

    void setConfig(const Config& config); // thread-safe; applies live
    void kick();                          // wake from backoff (device/power events)

private:
    void run();
    bool session();

    Logger*           log_;
    std::thread       thread_;
    std::atomic<bool> running_{false};
    VeyraWinHandle    kickEvent_ = nullptr;

    std::mutex mutex_;
    Config     cfg_;
    bool       haveCfg_ = false;
};

} // namespace veyra::service
