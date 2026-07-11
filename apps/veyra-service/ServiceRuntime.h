#pragma once

#include "ApoPublisher.h"
#include "AudioBridge.h"
#include "ConfigManager.h"
#include "ControlServer.h"
#include "DeviceNotifier.h"
#include "MicBridge.h"
#include "MicPublisher.h"
#include "PresetLibrary.h"
#include "SleepTimerService.h"
#include "TrackerService.h"
#include "UpdaterClient.h"
#include "veyra/Logging.h"

namespace veyra::service {

// The service's actual work, independent of how it was launched. Both the SCM
// dispatcher (ServiceMain) and console mode (--console) drive this object, so
// the runtime logic is written once.
class ServiceRuntime {
public:
    // consoleLogging echoes the log to stdout (the --console debug mode).
    explicit ServiceRuntime(bool consoleLogging = false);

    bool start(); // load config + start the control server
    void stop();

    // Suspend/resume (SERVICE_CONTROL_POWEREVENT): restart the audio sessions
    // promptly instead of waiting out their backoff.
    void onPowerEvent();

    Logger& log() { return log_; }

private:
    Logger            log_;
    ApoPublisher      publisher_;
    MicPublisher      micPublisher_;
    TrackerService    tracker_;
    SleepTimerService sleepTimer_;
    AudioBridge       bridge_;
    MicBridge         micBridge_;
    ConfigManager     config_;
    PresetLibrary     presets_;
    ControlServer     control_;
    UpdaterClient     updater_;
    DeviceNotifier    devices_;
};

} // namespace veyra::service
