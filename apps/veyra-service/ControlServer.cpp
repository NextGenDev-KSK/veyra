#include "ControlServer.h"

#include "ConfigManager.h"
#include "veyra/Logging.h"
#include "veyra/ipc/Protocol.h"
#include "veyra/version.h"

namespace veyra::service {

using ipc::Message;
using ipc::MessageType;

ControlServer::ControlServer(ConfigManager& config, Logger* log)
    : config_(config),
      log_(log),
      server_(ipc::kControlPipeName,
              [this](const Message& req) { return handle(req); },
              log)
{
}

Message ControlServer::handle(const Message& request)
{
    switch (request.type)
    {
    case MessageType::Ping:
        return {MessageType::Pong, {}};

    case MessageType::GetVersion:
        return {MessageType::VersionReply, kVersionString};

    case MessageType::GetConfig:
        return {MessageType::ConfigReply, config_.getJson()};

    case MessageType::SetConfig:
        if (config_.setJson(request.payload))
            return {MessageType::SetConfigAck, {}};
        return {MessageType::Error, "invalid config payload"};

    default:
        if (log_)
            log_->warn("ControlServer: unknown message type");
        return {MessageType::Error, "unknown message type"};
    }
}

} // namespace veyra::service
