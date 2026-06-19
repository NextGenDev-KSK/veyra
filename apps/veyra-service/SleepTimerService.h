#pragma once

// Sleep timer: when enabled, fades the default render endpoint's master volume
// to silence over the configured tail (driven by the tested dsp::SleepTimer),
// then restores the original level when disabled or on shutdown. This works
// without the APO — it ramps the system volume directly — so the feature is
// usable in the everyday (no-driver) setup.

#include <atomic>
#include <mutex>
#include <thread>

#include "veyra/Config.h"

namespace veyra {
class Logger;
}

namespace veyra::service {

class SleepTimerService {
public:
    explicit SleepTimerService(Logger* log = nullptr) : log_(log) {}
    ~SleepTimerService();

    bool start();
    void stop();

    // Apply loudness/sleep settings (thread-safe).
    void setConfig(const LoudnessConfig& loud);

private:
    void run();

    Logger*           log_;
    std::thread       thread_;
    std::atomic<bool> running_{false};

    std::mutex     mutex_;
    LoudnessConfig cfg_;
};

} // namespace veyra::service
