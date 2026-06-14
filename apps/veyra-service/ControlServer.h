#pragma once

#include "veyra/ipc/PipeServer.h"

namespace veyra {
class Logger;
}

namespace veyra::service {

class ConfigManager;

// Serves the \\.\pipe\veyra-control channel: ping/pong, version, and config
// get/set, backed by the ConfigManager.
class ControlServer {
public:
    ControlServer(ConfigManager& config, Logger* log);

    bool start() { return server_.start(); }
    void stop()  { server_.stop(); }

private:
    ipc::Message handle(const ipc::Message& request);

    ConfigManager&   config_;
    Logger*          log_;
    ipc::PipeServer  server_;
};

} // namespace veyra::service
