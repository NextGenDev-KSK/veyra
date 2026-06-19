#pragma once

// Gamer Mode producer: a WASAPI loopback capture of the default render endpoint
// feeding veyra::dsp::SoundTracker, publishing detections to the VeyraTrackerData
// shared block the overlay reads. Runs on its own thread; capture only happens
// while Gamer Mode is enabled. No graphics-API hooking — fully anti-cheat safe.

#include <atomic>
#include <thread>

#include "analyzer/Analyzer.h"
#include "veyra/Config.h"
#include "veyra/ipc/AnalyzerData.h"
#include "veyra/ipc/SharedMemory.h"
#include "veyra/ipc/TrackerData.h"

namespace veyra {
class Logger;
}

namespace veyra::service {

class TrackerService {
public:
    explicit TrackerService(Logger* log = nullptr) : log_(log) {}
    ~TrackerService();

    bool start();
    void stop();

    // Apply Gamer Mode settings (thread-safe).
    void setConfig(const GamerModeConfig& gm);

private:
    void run();             // capture thread
    bool captureSession();  // one WASAPI session; returns false to retry later
    void publishAnalyzerFrame(const dsp::AnalyzerFrame& frame); // -> analyzer block

    Logger*                        log_;
    ipc::SharedMemoryRegion        region_;
    ipc::VeyraTrackerData*         data_ = nullptr;
    ipc::SharedMemoryRegion        analyzerRegion_;
    ipc::VeyraAnalyzerData*        analyzerData_ = nullptr;
    float                          specMax_ = 1.0e-6f; // auto-gain reference

    std::thread        thread_;
    std::atomic<bool>  running_{false};
    std::atomic<bool>  enabled_{false};   // Gamer Mode: gates tracker events only
    std::atomic<float> sensitivity_{0.6f};
};

} // namespace veyra::service
