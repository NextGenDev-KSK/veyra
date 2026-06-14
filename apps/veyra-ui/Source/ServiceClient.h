#pragma once

// Background connection to the Veyra service control pipe. Owns its own thread,
// performs the version handshake, keeps the connection alive with periodic
// pings, and auto-reconnects with exponential backoff. UI-thread code reads a
// thread-safe status snapshot and is nudged to repaint via a posted message.

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <string>
#include <thread>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

namespace veyra::ui {

enum class ConnectionState { Connecting, Connected, Reconnecting };

struct ClientStatus {
    ConnectionState state{ConnectionState::Connecting};
    std::wstring    serviceVersion; // valid when state == Connected
};

class ServiceClient {
public:
    ServiceClient() = default;
    ~ServiceClient();

    // Begin connecting. On any state change the client posts 'notifyMessage' to
    // 'notifyWindow'; the handler should call status() and repaint.
    void start(HWND notifyWindow, UINT notifyMessage);
    void stop();

    ClientStatus status();

private:
    void run();
    void setStatus(ConnectionState state, std::wstring version);
    bool sleepInterruptible(unsigned ms); // false if stop requested

    HWND   notifyWindow_  = nullptr;
    UINT   notifyMessage_ = 0;

    std::thread        thread_;
    std::atomic<bool>  running_{false};

    std::mutex   statusMutex_;
    ClientStatus status_;

    std::mutex              waitMutex_;
    std::condition_variable wait_;
};

} // namespace veyra::ui
