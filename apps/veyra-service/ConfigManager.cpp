#include "ConfigManager.h"

#include "veyra/Logging.h"

namespace veyra::service {

ConfigManager::ConfigManager(std::filesystem::path file, Logger* log)
    : file_(std::move(file)), log_(log)
{
}

void ConfigManager::loadOrCreateDefault()
{
    {
        std::lock_guard<std::mutex> lock(mutex_);

        if (auto loaded = Config::load(file_))
        {
            config_ = *loaded;
            if (log_)
                log_->info("ConfigManager: loaded config from disk");
        }
        else
        {
            // No (or unreadable) config — write defaults and verify the round
            // trip so a broken state surfaces immediately in the log.
            config_ = Config{};
            if (config_.save(file_))
            {
                if (auto check = Config::load(file_);
                    check && check->toJson() == config_.toJson())
                {
                    if (log_)
                        log_->info("ConfigManager: wrote default config; round-trip OK");
                }
                else if (log_)
                    log_->error("ConfigManager: default config round-trip MISMATCH");
            }
            else if (log_)
                log_->error("ConfigManager: failed to write default config");
        }
    }
    notifyChanged();
}

std::string ConfigManager::getJson()
{
    std::lock_guard<std::mutex> lock(mutex_);
    return config_.toJson();
}

bool ConfigManager::setJson(const std::string& json)
{
    auto parsed = Config::fromJson(json);
    if (!parsed)
    {
        if (log_)
            log_->warn("ConfigManager: rejected malformed SetConfig");
        return false;
    }

    bool saved = false;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        config_ = *parsed;
        saved = config_.save(file_);
        if (log_)
            log_->log(saved ? LogLevel::Info : LogLevel::Error,
                      saved ? "ConfigManager: applied + saved config"
                            : "ConfigManager: applied config but save FAILED");
    }
    notifyChanged();
    return saved;
}

void ConfigManager::notifyChanged()
{
    Config snapshot;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        snapshot = config_;
    }
    if (onChanged_)
        onChanged_(snapshot);
}

} // namespace veyra::service
