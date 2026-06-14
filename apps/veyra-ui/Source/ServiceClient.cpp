#include "ServiceClient.h"

#include <algorithm>
#include <chrono>

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

void ServiceClient::start(HWND notifyWindow, UINT notifyMessage)
{
    notifyWindow_  = notifyWindow;
    notifyMessage_ = notifyMessage;
    running_.store(true);
    thread_ = std::thread(&ServiceClient::run, this);
}

void ServiceClient::stop()
{
    if (!running_.exchange(false))
        return;
    wait_.notify_all();
    if (thread_.joinable())
        thread_.join();
}

ClientStatus ServiceClient::status()
{
    std::lock_guard<std::mutex> lock(statusMutex_);
    return status_;
}

void ServiceClient::setStatus(ConnectionState state, std::wstring version)
{
    {
        std::lock_guard<std::mutex> lock(statusMutex_);
        status_.state = state;
        status_.serviceVersion = std::move(version);
    }
    if (notifyWindow_)
        PostMessageW(notifyWindow_, notifyMessage_, 0, 0);
}

bool ServiceClient::sleepInterruptible(unsigned ms)
{
    std::unique_lock<std::mutex> lock(waitMutex_);
    const bool stopRequested = wait_.wait_for(
        lock, std::chrono::milliseconds(ms), [this] { return !running_.load(); });
    return !stopRequested;
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

                // Keepalive: ping until the connection drops or we're stopped.
                while (running_.load())
                {
                    if (!sleepInterruptible(kPingIntervalMs))
                        break; // stop requested
                    Message pong;
                    if (!client.request({MessageType::Ping, {}}, pong) ||
                        pong.type != MessageType::Pong)
                        break; // connection lost
                }
                client.close();
                continue; // loop back -> Reconnecting
            }
            client.close();
        }

        firstAttempt = false;
        if (!sleepInterruptible(backoff))
            break;
        backoff = std::min(backoff * 2, kBackoffMaxMs);
    }
}

} // namespace veyra::ui
