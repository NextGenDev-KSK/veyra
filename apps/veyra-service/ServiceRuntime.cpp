#include "ServiceRuntime.h"

#include "veyra/Paths.h"
#include "veyra/version.h"

namespace veyra::service {

ServiceRuntime::ServiceRuntime()
    : log_(paths::logsDir() / "veyra-service.log"),
      config_(paths::configFile(), &log_),
      control_(config_, &log_)
{
}

bool ServiceRuntime::start()
{
    log_.info(std::string("Veyra service starting, v") + kVersionString);

    config_.loadOrCreateDefault();

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
