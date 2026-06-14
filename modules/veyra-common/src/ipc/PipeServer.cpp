#include "veyra/ipc/PipeServer.h"

#include "veyra/Logging.h"
#include "veyra/ipc/PipeIo.h"

namespace veyra::ipc {

namespace {
constexpr DWORD kPipeBuffer = 64 * 1024;

void logLine(Logger* log, LogLevel level, std::string_view msg)
{
    if (log)
        log->log(level, msg);
}
} // namespace

PipeServer::PipeServer(std::wstring pipeName, RequestHandler handler, Logger* log)
    : pipeName_(std::move(pipeName)), handler_(std::move(handler)), log_(log)
{
}

PipeServer::~PipeServer()
{
    stop();
}

bool PipeServer::start()
{
    if (running_.exchange(true))
        return false; // already running

    listener_ = std::thread(&PipeServer::listenLoop, this);
    return true;
}

void PipeServer::stop()
{
    if (!running_.exchange(false))
        return; // not running

    // Abort the blocking ConnectNamedPipe / ReadFile the listener is parked in.
    {
        std::lock_guard<std::mutex> lock(threadHandleMutex_);
        if (listenerThread_)
            CancelSynchronousIo(listenerThread_);
    }

    if (listener_.joinable())
        listener_.join();

    std::lock_guard<std::mutex> lock(threadHandleMutex_);
    if (listenerThread_)
    {
        CloseHandle(listenerThread_);
        listenerThread_ = nullptr;
    }
}

void PipeServer::listenLoop()
{
    // Duplicate our own thread handle so stop() can target it precisely.
    {
        std::lock_guard<std::mutex> lock(threadHandleMutex_);
        DuplicateHandle(GetCurrentProcess(), GetCurrentThread(),
                        GetCurrentProcess(), &listenerThread_,
                        0, FALSE, DUPLICATE_SAME_ACCESS);
    }

    while (running_.load())
    {
        HANDLE pipe = CreateNamedPipeW(
            pipeName_.c_str(),
            PIPE_ACCESS_DUPLEX,
            PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
            PIPE_UNLIMITED_INSTANCES,
            kPipeBuffer, kPipeBuffer, 0, nullptr);

        if (pipe == INVALID_HANDLE_VALUE)
        {
            logLine(log_, LogLevel::Error, "PipeServer: CreateNamedPipe failed");
            break;
        }

        const BOOL connected =
            ConnectNamedPipe(pipe, nullptr) ? TRUE
                                            : (GetLastError() == ERROR_PIPE_CONNECTED);

        if (!running_.load())
        {
            CloseHandle(pipe);
            break;
        }

        if (connected)
            serveClient(pipe);

        CloseHandle(pipe);
    }
}

void PipeServer::serveClient(HANDLE pipe)
{
    logLine(log_, LogLevel::Info, "PipeServer: client connected");

    while (running_.load())
    {
        Message request;
        if (!readMessage(pipe, request))
            break; // disconnect or aborted

        Message reply = handler_(request);
        if (!writeMessage(pipe, reply))
            break;
    }

    FlushFileBuffers(pipe);
    DisconnectNamedPipe(pipe);
    logLine(log_, LogLevel::Info, "PipeServer: client disconnected");
}

} // namespace veyra::ipc
