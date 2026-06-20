#include "ServiceRuntime.h"

#include "veyra/CrashReport.h"
#include "veyra/Paths.h"
#include "veyra/version.h"

namespace veyra::service {

ServiceRuntime::ServiceRuntime(bool consoleLogging)
    : log_(paths::logsDir() / "veyra-service.log", consoleLogging),
      publisher_(&log_),
      micPublisher_(&log_),
      tracker_(&log_),
      sleepTimer_(&log_),
      bridge_(&log_),
      config_(paths::configFile(), &log_),
      presets_(paths::presetsDir(), &log_),
      control_(config_, presets_, paths::appDataDir() / "app_rules.json", &log_)
{
}

bool ServiceRuntime::start()
{
    log_.info(std::string("Veyra service starting, v") + kVersionString);

    // Surface a crash from a previous session (the UI shows a banner; the
    // service notes it in the log).
    if (auto crash = latestCrashReport(paths::crashesDir()))
        log_.warn("Previous session crashed at " + crash->timestamp + " (" + crash->message + ")");

    // Stand up the APO shared-memory parameter block, then republish whenever
    // the config changes so the APO always reflects current settings.
    publisher_.start();
    micPublisher_.start();
    tracker_.start();     // Gamer Mode loopback capture + Sound Tracker producer
    sleepTimer_.start();  // sleep-timer endpoint-volume fade
    bridge_.start();      // no-driver processing path (loopback -> DSP -> output)
    config_.setOnChanged([this](const Config& c)
    {
        publisher_.publish(c);
        micPublisher_.publish(c);          // mic chain params for the capture APO
        tracker_.setConfig(c.gamerMode);   // enable/sensitivity for the tracker
        sleepTimer_.setConfig(c.loudness); // sleep timer enable/duration
        bridge_.setConfig(c);              // bridge routing + live DSP params
    });

    presets_.load();        // built-ins + user .vpreset files
    control_.loadAppRules(); // persisted per-app rules

    config_.loadOrCreateDefault(); // triggers the first publish

    if (!control_.start())
    {
        log_.error("Failed to start control server");
        return false;
    }
    log_.info("Control server listening on \\\\.\\pipe\\veyra-control");
    return true;
}

void ServiceRuntime::stop()
{
    control_.stop();
    tracker_.stop();
    sleepTimer_.stop(); // restores the endpoint volume if a fade was in progress
    bridge_.stop();
    log_.info("Veyra service stopped");
}

} // namespace veyra::service
