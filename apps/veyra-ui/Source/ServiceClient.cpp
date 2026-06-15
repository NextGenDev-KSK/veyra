#include "ServiceClient.h"

#include <algorithm>
#include <chrono>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

#include "veyra/ipc/PipeClient.h"
#include "veyra/ipc/Protocol.h"

namespace veyra::ui {

namespace {

constexpr unsigned kConnectTimeoutMs = 800;
constexpr unsigned kPingIntervalMs   = 2000;
constexpr unsigned kBackoffStartMs   = 500;
constexpr unsigned kBackoffMaxMs     = 30000;

std::wstring utf8ToWide(const std::string& s)
{
    if (s.empty())
        return {};
    const int n = MultiByteToWideChar(CP_UTF8, 0, s.data(), static_cast<int>(s.size()),
                                      nullptr, 0);
    std::wstring out(static_cast<size_t>(n), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, s.data(), static_cast<int>(s.size()), out.data(), n);
    return out;
}

} // namespace

ServiceClient::~ServiceClient()
{
    stop();
}

void ServiceClient::start(ChangeCallback onChange)
{
    onChange_ = std::move(onChange);
    running_.store(true);
    thread_ = std::thread(&ServiceClient::run, this);
}

void ServiceClient::stop()
{
    if (!running_.exchange(false))
        return;
    {
        std::lock_guard<std::mutex> lock(waitMutex_);
        workPending_ = true;
    }
    wait_.notify_all();
    if (thread_.joinable())
        thread_.join();
}

ClientStatus ServiceClient::status()
{
    std::lock_guard<std::mutex> lock(statusMutex_);
    return status_;
}

std::optional<veyra::Config> ServiceClient::config()
{
    std::lock_guard<std::mutex> lock(stateMutex_);
    return config_;
}

void ServiceClient::updateConfig(const veyra::Config& config)
{
    {
        std::lock_guard<std::mutex> lock(stateMutex_);
        desiredConfig_ = config;
        configDirty_   = true;
    }
    {
        std::lock_guard<std::mutex> lock(waitMutex_);
        workPending_ = true;
    }
    wait_.notify_all();
}

void ServiceClient::setStatus(ConnectionState state, std::wstring version)
{
    {
        std::lock_guard<std::mutex> lock(statusMutex_);
        status_.state = state;
        status_.serviceVersion = std::move(version);
    }
    if (onChange_)
        onChange_();
}

bool ServiceClient::waitForWork(unsigned ms)
{
    std::unique_lock<std::mutex> lock(waitMutex_);
    wait_.wait_for(lock, std::chrono::milliseconds(ms),
                   [this] { return !running_.load() || workPending_; });
    workPending_ = false;
    return running_.load();
}

void ServiceClient::run()
{
    using namespace veyra::ipc;

    PipeClient client(kControlPipeName);
    unsigned backoff = kBackoffStartMs;
    bool firstAttempt = true;

    while (running_.load())
    {
        setStatus(firstAttempt ? ConnectionState::Connecting
                               : ConnectionState::Reconnecting,
                  {});

        if (client.connect(kConnectTimeoutMs))
        {
            Message reply;
            if (client.request({MessageType::GetVersion, {}}, reply) &&
                reply.type == MessageType::VersionReply)
            {
                setStatus(ConnectionState::Connected, utf8ToWide(reply.payload));
                backoff = kBackoffStartMs;
                firstAttempt = false;

                // Adopt the service's persisted config as our baseline so we
                // don't immediately overwrite it with a default on connect.
                Message cfg;
                if (client.request({MessageType::GetConfig, {}}, cfg) &&
                    cfg.type == MessageType::ConfigReply)
                {
                    if (auto parsed = veyra::Config::fromJson(cfg.payload))
                    {
                        std::lock_guard<std::mutex> lock(stateMutex_);
                        config_ = *parsed;
                        if (!configDirty_) // keep any unsent user changes
                            desiredConfig_ = *parsed;
                    }
                    if (onChange_)
                        onChange_();
                }

                // Keepalive: send coalesced config changes, otherwise ping.
                bool ok = true;
                while (running_.load() && ok)
                {
                    if (!waitForWork(kPingIntervalMs))
                        break; // stop requested

                    std::optional<veyra::Config> toSend;
                    {
                        std::lock_guard<std::mutex> lock(stateMutex_);
                        if (configDirty_)
                        {
                            toSend       = desiredConfig_;
                            configDirty_ = false;
                        }
                    }

                    if (toSend)
                    {
                        Message ack;
                        ok = client.request({MessageType::SetConfig, toSend->toJson()}, ack) &&
                             ack.type == MessageType::SetConfigAck;
                        if (ok)
                        {
                            std::lock_guard<std::mutex> lock(stateMutex_);
                            config_ = *toSend;
                        }
                    }
                    else
                    {
                        Message pong;
                        ok = client.request({MessageType::Ping, {}}, pong) &&
                             pong.type == MessageType::Pong;
                    }
                }
                client.close();
                continue; // loop back -> Reconnecting
            }
            client.close();
        }

        firstAttempt = false;
        {
            // Backoff wait, interruptible by stop().
            std::unique_lock<std::mutex> lock(waitMutex_);
            wait_.wait_for(lock, std::chrono::milliseconds(backoff),
                           [this] { return !running_.load(); });
            workPending_ = false;
        }
        if (!running_.load())
            break;
        backoff = std::min(backoff * 2, kBackoffMaxMs);
    }
}

} // namespace veyra::ui
