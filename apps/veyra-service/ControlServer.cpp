#include "ControlServer.h"

#include <fstream>
#include <sstream>

#include "ConfigManager.h"
#include "PresetLibrary.h"
#include "veyra/Logging.h"
#include "veyra/ipc/Protocol.h"
#include "veyra/version.h"

namespace veyra::service {

using ipc::Message;
using ipc::MessageType;

ControlServer::ControlServer(ConfigManager& config, PresetLibrary& presets,
                             std::filesystem::path appRulesFile, Logger* log)
    : config_(config),
      presets_(presets),
      appRulesFile_(std::move(appRulesFile)),
      log_(log),
      server_(ipc::kControlPipeName,
              [this](const Message& req) { return handle(req); },
              log)
{
}

void ControlServer::loadAppRules()
{
    std::ifstream in(appRulesFile_, std::ios::binary);
    if (!in)
        return;
    std::ostringstream ss;
    ss << in.rdbuf();
    rules_.setRules(AppRuleEngine::rulesFromJson(ss.str()));
    if (log_)
        log_->info("ControlServer: loaded " + std::to_string(rules_.rules().size()) + " app rule(s)");
}

void ControlServer::persistAppRules()
{
    std::error_code ec;
    std::filesystem::create_directories(appRulesFile_.parent_path(), ec);
    std::ofstream out(appRulesFile_, std::ios::binary | std::ios::trunc);
    if (out)
        out << rules_.toJson();
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

    case MessageType::ListPresets:
        return {MessageType::PresetsReply, presets_.toJsonArray()};

    case MessageType::LoadPreset:
        if (auto p = presets_.find(request.payload); p && config_.applyPreset(*p))
            return {MessageType::PresetAck, {}};
        return {MessageType::Error, "unknown or unapplied preset"};

    case MessageType::SavePreset:
        if (auto p = Preset::fromJson(request.payload); p && presets_.saveUser(*p))
            return {MessageType::PresetAck, {}};
        return {MessageType::Error, "invalid or unsavable preset"};

    case MessageType::DeletePreset:
        if (presets_.removeUser(request.payload))
            return {MessageType::PresetAck, {}};
        return {MessageType::Error, "preset not found or built-in"};

    case MessageType::GetAppRules:
        return {MessageType::AppRulesReply, rules_.toJson()};

    case MessageType::SetAppRules:
        rules_.setRules(AppRuleEngine::rulesFromJson(request.payload));
        persistAppRules();
        return {MessageType::AppRulesAck, {}};

    default:
        if (log_)
            log_->warn("ControlServer: unknown message type");
        return {MessageType::Error, "unknown message type"};
    }
}

} // namespace veyra::service
