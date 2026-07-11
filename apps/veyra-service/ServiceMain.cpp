#include "ServiceMain.h"

#include "ServiceInstaller.h"
#include "ServiceRuntime.h"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

namespace veyra::service {

namespace {

SERVICE_STATUS_HANDLE g_statusHandle = nullptr;
SERVICE_STATUS        g_status{};
HANDLE                g_stopEvent = nullptr;
DWORD                 g_checkPoint = 1;
ServiceRuntime*       g_runtime = nullptr; // valid only while RUNNING

void reportStatus(DWORD state, DWORD waitHintMs = 0, DWORD win32ExitCode = NO_ERROR)
{
    g_status.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
    g_status.dwCurrentState = state;
    g_status.dwWin32ExitCode = win32ExitCode;
    g_status.dwWaitHint = waitHintMs;
    g_status.dwControlsAccepted =
        (state == SERVICE_START_PENDING)
            ? 0
            : (SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_SHUTDOWN | SERVICE_ACCEPT_POWEREVENT);

    if (state == SERVICE_RUNNING || state == SERVICE_STOPPED)
        g_status.dwCheckPoint = 0;
    else
        g_status.dwCheckPoint = g_checkPoint++;

    SetServiceStatus(g_statusHandle, &g_status);
}

DWORD WINAPI handlerEx(DWORD control, DWORD eventType, LPVOID, LPVOID)
{
    switch (control)
    {
    case SERVICE_CONTROL_STOP:
    case SERVICE_CONTROL_SHUTDOWN:
        reportStatus(SERVICE_STOP_PENDING, 3000);
        if (g_stopEvent)
            SetEvent(g_stopEvent);
        return NO_ERROR;

    case SERVICE_CONTROL_POWEREVENT:
        // After sleep/resume the audio sessions restart promptly instead of
        // waiting out their retry backoff.
        if ((eventType == PBT_APMRESUMEAUTOMATIC || eventType == PBT_APMRESUMESUSPEND)
            && g_runtime != nullptr)
            g_runtime->onPowerEvent();
        return NO_ERROR;

    case SERVICE_CONTROL_INTERROGATE:
        return NO_ERROR;

    default:
        return ERROR_CALL_NOT_IMPLEMENTED;
    }
}

VOID WINAPI serviceMain(DWORD, LPWSTR*)
{
    g_statusHandle = RegisterServiceCtrlHandlerExW(kServiceName, handlerEx, nullptr);
    if (!g_statusHandle)
        return;

    reportStatus(SERVICE_START_PENDING, 3000);

    g_stopEvent = CreateEventW(nullptr, TRUE, FALSE, nullptr);
    if (!g_stopEvent)
    {
        // Report the real error so SCM failure actions fire and the failure is
        // visible in the event log, rather than looking like a clean stop.
        reportStatus(SERVICE_STOPPED, 0, GetLastError());
        return;
    }

    ServiceRuntime runtime;
    if (!runtime.start())
    {
        CloseHandle(g_stopEvent);
        g_stopEvent = nullptr;
        g_status.dwServiceSpecificExitCode = 1; // runtime failed to start (see service log)
        reportStatus(SERVICE_STOPPED, 0, ERROR_SERVICE_SPECIFIC_ERROR);
        return;
    }

    g_runtime = &runtime;
    reportStatus(SERVICE_RUNNING);
    WaitForSingleObject(g_stopEvent, INFINITE);

    g_runtime = nullptr;
    runtime.stop();
    CloseHandle(g_stopEvent);
    g_stopEvent = nullptr;
    reportStatus(SERVICE_STOPPED);
}

} // namespace

int runAsService()
{
    SERVICE_TABLE_ENTRYW table[] = {
        {const_cast<LPWSTR>(kServiceName), serviceMain},
        {nullptr, nullptr},
    };

    if (!StartServiceCtrlDispatcherW(table))
        return static_cast<int>(GetLastError());
    return 0;
}

} // namespace veyra::service
