#include "veyra/ipc/PipeClient.h"

#include "veyra/ipc/PipeIo.h"

namespace veyra::ipc {

PipeClient::PipeClient(std::wstring pipeName)
    : pipeName_(std::move(pipeName))
{
}

PipeClient::~PipeClient()
{
    close();
}

bool PipeClient::connect(DWORD timeoutMs)
{
    close();

    for (;;)
    {
        pipe_ = CreateFileW(pipeName_.c_str(),
                            GENERIC_READ | GENERIC_WRITE,
                            0, nullptr, OPEN_EXISTING, 0, nullptr);

        if (pipe_ != INVALID_HANDLE_VALUE)
            break;

        if (GetLastError() != ERROR_PIPE_BUSY)
            return false;

        // Server exists but every instance is busy — wait for a free one.
        if (!WaitNamedPipeW(pipeName_.c_str(), timeoutMs))
            return false;
    }

    // Match the server's message framing.
    DWORD mode = PIPE_READMODE_MESSAGE;
    if (!SetNamedPipeHandleState(pipe_, &mode, nullptr, nullptr))
    {
        close();
        return false;
    }
    return true;
}

void PipeClient::close()
{
    if (pipe_ != INVALID_HANDLE_VALUE)
    {
        CloseHandle(pipe_);
        pipe_ = INVALID_HANDLE_VALUE;
    }
}

bool PipeClient::request(const Message& req, Message& reply)
{
    if (!isConnected())
        return false;

    if (!writeMessage(pipe_, req) || !readMessage(pipe_, reply))
    {
        close();
        return false;
    }
    return true;
}

} // namespace veyra::ipc
