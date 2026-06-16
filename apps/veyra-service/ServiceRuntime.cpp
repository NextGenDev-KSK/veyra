#include "ServiceRuntime.h"

#include "veyra/Paths.h"
#include "veyra/version.h"

namespace veyra::service {

ServiceRuntime::ServiceRuntime()
    : log_(paths::logsDir() / "veyra-service.log"),
      publisher_(&log_),
      config_(paths::configFile(), &log_),
      presets_(paths::presetsDir(), &log_),
      control_(config_, presets_, paths::appDataDir() / "app_rules.json", &log_)
{
}

bool ServiceRuntime::start()
{
    log_.info(std::string("Veyra service starting, v") + kVersionString);

    // Stand up the APO shared-memory parameter block, then republish whenever
    // the config changes so the APO always reflects current settings.
    publisher_.start();
    config_.setOnChanged([this](const Config& c) { publisher_.publish(c); });

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
    log_.info("Veyra service stopped");
}

} // namespace veyra::service
