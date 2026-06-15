#pragma once

// Background connection to the Veyra service control pipe. Owns its own thread,
// performs the version handshake, fetches the live config, keeps the connection
// alive with periodic pings, and auto-reconnects with exponential backoff.
//
// The UI pushes parameter changes through updateConfig(): the desired config is
// stored and coalesced, and the background thread sends the latest snapshot as a
// SetConfig as soon as it can (so a fast drag turns into a handful of writes,
// not one per pixel). On any connection/config change the client invokes the
// onChange callback on its own thread; the UI marshals that to the message
// thread before touching components.

#include <atomic>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <optional>
#include <string>
#include <thread>

#include "veyra/Config.h"

namespace veyra::ui {

enum class ConnectionState { Connecting, Connected, Reconnecting };

struct ClientStatus {
    ConnectionState state{ConnectionState::Connecting};
    std::wstring    serviceVersion; // valid when state == Connected
};

class ServiceClient {
public:
    using ChangeCallback = std::function<void()>;

    ServiceClient() = default;
    ~ServiceClient();

    // Begin connecting. 'onChange' is called (on the background thread) whenever
    // the connection state changes or a fresh config is fetched from the service.
    void start(ChangeCallback onChange);
    void stop();

    ClientStatus status();

    // Last config known from the service. std::nullopt until the first
    // successful fetch.
    std::optional<veyra::Config> config();

    // Push a desired config to the service. Thread-safe and coalescing: only the
    // most recent value is sent. Safe to call before a connection exists.
    void updateConfig(const veyra::Config& config);

private:
    void run();
    void setStatus(ConnectionState state, std::wstring version);
    bool waitForWork(unsigned ms); // false if stop requested

    ChangeCallback onChange_;

    std::thread       thread_;
    std::atomic<bool> running_{false};

    std::mutex   statusMutex_;
    ClientStatus status_;

    std::mutex                   stateMutex_;
    std::optional<veyra::Config> config_;        // last fetched from service
    veyra::Config                desiredConfig_;  // latest UI intent
    bool                         configDirty_ = false;

    std::mutex              waitMutex_;
    std::condition_variable wait_;
    bool                    workPending_ = false; // guarded by waitMutex_
};

} // namespace veyra::ui
