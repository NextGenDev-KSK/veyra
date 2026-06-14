#pragma once

// A single-listener named-pipe server. One client is served at a time, which
// is all the control channel needs (the UI is the sole control client); when a
// client disconnects the listener loops and accepts the next connection.
//
// Shutdown is clean: stop() aborts whatever blocking call the listener thread
// is in (ConnectNamedPipe or ReadFile) via CancelSynchronousIo, so there are no
// dummy self-connections and no leaked threads.

#include <atomic>
#include <functional>
#include <mutex>
#include <string>
#include <thread>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

#include "veyra/ipc/Protocol.h"

namespace veyra {
class Logger;
}

namespace veyra::ipc {

class PipeServer {
public:
    // Returns the reply to send for a given request. Runs on the listener
    // thread; keep it quick and thread-safe with respect to shared state.
    using RequestHandler = std::function<Message(const Message& request)>;

    PipeServer(std::wstring pipeName, RequestHandler handler, Logger* log = nullptr);
    ~PipeServer();

    PipeServer(const PipeServer&) = delete;
    PipeServer& operator=(const PipeServer&) = delete;

    bool start();
    void stop();

private:
    void listenLoop();
    void serveClient(HANDLE pipe);

    std::wstring     pipeName_;
    RequestHandler   handler_;
    Logger*          log_;

    std::thread        listener_;
    std::atomic<bool>  running_{false};

    std::mutex  threadHandleMutex_;
    HANDLE      listenerThread_{nullptr}; // duplicated handle for CancelSynchronousIo
};

} // namespace veyra::ipc
