#include "veyra/ipc/PipeServer.h"

#include <sddl.h>

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

// Build a SECURITY_ATTRIBUTES for the named pipe that restricts connections to:
//   SY  LocalSystem (the service)
//   BA  Built-in Administrators
//   IU  Interactive Users (the logged-on user running the UI / overlay)
// Callers must call LocalFree(sd) after CreateNamedPipeW returns.
bool buildPipeSa(SECURITY_ATTRIBUTES& sa, PSECURITY_DESCRIPTOR& sd)
{
    static constexpr wchar_t kSddl[] =
        L"D:(A;;FA;;;SY)(A;;FA;;;BA)(A;;GRGW;;;IU)";
    if (!ConvertStringSecurityDescriptorToSecurityDescriptorW(
            kSddl, SDDL_REVISION_1, &sd, nullptr))
        return false;
    sa.nLength              = sizeof(sa);
    sa.lpSecurityDescriptor = sd;
    sa.bInheritHandle       = FALSE;
    return true;
}

// Returns true if the connected client is in a trusted session.
//
// Named pipes are always local — no remote-connection risk. We additionally
// restrict to Session 0 (LocalService self-connections) and the active console
// session (the interactive user running the UI / overlay). This rejects RDP
// users logged into other sessions from manipulating another user's audio.
//
// GetNamedPipeClientSessionId and WTSGetActiveConsoleSessionId are in
// Kernel32.dll (Vista+); no extra library dependency.
bool isAuthorizedClient(HANDLE pipe, Logger* log)
{
    ULONG clientSession = 0;
    if (!GetNamedPipeClientSessionId(pipe, &clientSession))
    {
        logLine(log, LogLevel::Warn,
                "PipeServer: GetNamedPipeClientSessionId failed — rejecting");
        return false;
    }

    ULONG pid = 0;
    GetNamedPipeClientProcessId(pipe, &pid); // best-effort; ignore failure

    const DWORD consoleSession = WTSGetActiveConsoleSessionId();
    if (clientSession != 0 && clientSession != consoleSession)
    {
        logLine(log, LogLevel::Warn,
                "PipeServer: rejected pid=" + std::to_string(pid) +
                " session=" + std::to_string(clientSession) +
                " (active console session=" + std::to_string(consoleSession) + ")");
        return false;
    }

    logLine(log, LogLevel::Info,
            "PipeServer: accepted pid=" + std::to_string(pid) +
            " session=" + std::to_string(clientSession));
    return true;
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
        PSECURITY_DESCRIPTOR sd  = nullptr;
        SECURITY_ATTRIBUTES  sa{};
        SECURITY_ATTRIBUTES* pSa = buildPipeSa(sa, sd) ? &sa : nullptr;

        HANDLE pipe = CreateNamedPipeW(
            pipeName_.c_str(),
            PIPE_ACCESS_DUPLEX,
            PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
            PIPE_UNLIMITED_INSTANCES,
            kPipeBuffer, kPipeBuffer, 0, pSa);

        if (sd)
        {
            LocalFree(sd);
            sd = nullptr;
        }

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
        {
            if (isAuthorizedClient(pipe, log_))
                serveClient(pipe);
            else
                DisconnectNamedPipe(pipe);
        }

        CloseHandle(pipe);
    }
}

void PipeServer::serveClient(HANDLE pipe)
{
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
