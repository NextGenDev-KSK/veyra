#pragma once

#include "ConfigManager.h"
#include "ControlServer.h"
#include "veyra/Logging.h"

namespace veyra::service {

// The service's actual work, independent of how it was launched. Both the SCM
// dispatcher (ServiceMain) and console mode (--console) drive this object, so
// the runtime logic is written once.
class ServiceRuntime {
public:
    ServiceRuntime();

    bool start(); // load config + start the control server
    void stop();

    Logger& log() { return log_; }

private:
    Logger        log_;
    ConfigManager config_;
    ControlServer control_;
};

} // namespace veyra::service
