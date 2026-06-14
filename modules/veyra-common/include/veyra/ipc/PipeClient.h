#pragma once

// Thin synchronous client for the Veyra named-pipe protocol. Callers own the
// connection lifecycle and any reconnect policy (see the UI's ServiceClient).

#include <string>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

#include "veyra/ipc/Protocol.h"

namespace veyra::ipc {

class PipeClient {
public:
    explicit PipeClient(std::wstring pipeName);
    ~PipeClient();

    PipeClient(const PipeClient&) = delete;
    PipeClient& operator=(const PipeClient&) = delete;

    // Attempts to connect, waiting up to timeoutMs for a busy server instance.
    bool connect(DWORD timeoutMs = 1000);
    void close();
    bool isConnected() const { return pipe_ != INVALID_HANDLE_VALUE; }

    // Send a request and block for the reply. Closes the connection on failure.
    bool request(const Message& req, Message& reply);

private:
    std::wstring pipeName_;
    HANDLE       pipe_{INVALID_HANDLE_VALUE};
};

} // namespace veyra::ipc
