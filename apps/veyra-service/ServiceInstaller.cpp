#include "ServiceInstaller.h"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

namespace veyra::service {

namespace {

std::wstring lastErrorText()
{
    const DWORD code = GetLastError();
    LPWSTR buf = nullptr;
    const DWORD n = FormatMessageW(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |
            FORMAT_MESSAGE_IGNORE_INSERTS,
        nullptr, code, 0, reinterpret_cast<LPWSTR>(&buf), 0, nullptr);

    std::wstring msg = n && buf ? std::wstring(buf, n) : L"unknown error";
    if (buf)
        LocalFree(buf);
    // Trim trailing CRLF the formatter appends.
    while (!msg.empty() && (msg.back() == L'\r' || msg.back() == L'\n'))
        msg.pop_back();
    return L"(" + std::to_wstring(code) + L") " + msg;
}

std::wstring quotedModulePath()
{
    wchar_t path[MAX_PATH];
    GetModuleFileNameW(nullptr, path, MAX_PATH);
    return L"\"" + std::wstring(path) + L"\"";
}

} // namespace

bool installService(std::wstring& error)
{
    SC_HANDLE scm = OpenSCManagerW(nullptr, nullptr, SC_MANAGER_CREATE_SERVICE);
    if (!scm)
    {
        error = L"OpenSCManager failed " + lastErrorText();
        return false;
    }

    const std::wstring binPath = quotedModulePath();
    SC_HANDLE svc = CreateServiceW(
        scm, kServiceName, kDisplayName,
        SERVICE_CHANGE_CONFIG | SERVICE_START,
        SERVICE_WIN32_OWN_PROCESS, SERVICE_AUTO_START, SERVICE_ERROR_NORMAL,
        binPath.c_str(),
        nullptr, nullptr, nullptr,
        L"NT AUTHORITY\\LocalService", // reduced-privilege built-in account
        nullptr);                       // password: null for built-in accounts

    if (!svc)
    {
        // On upgrade the service already exists. Open it and update the binary
        // path so the new version is used on next start.
        if (GetLastError() != ERROR_SERVICE_EXISTS)
        {
            error = L"CreateService failed " + lastErrorText();
            CloseServiceHandle(scm);
            return false;
        }

        svc = OpenServiceW(scm, kServiceName, SERVICE_CHANGE_CONFIG | SERVICE_START);
        if (!svc)
        {
            error = L"OpenService (upgrade) failed " + lastErrorText();
            CloseServiceHandle(scm);
            return false;
        }

        if (!ChangeServiceConfigW(svc,
                SERVICE_NO_CHANGE,          // dwServiceType
                SERVICE_AUTO_START,         // dwStartType
                SERVICE_ERROR_NORMAL,       // dwErrorControl
                binPath.c_str(),            // lpBinaryPathName — update to new path
                nullptr, nullptr, nullptr,
                L"NT AUTHORITY\\LocalService",
                nullptr, nullptr))
        {
            error = L"ChangeServiceConfig (upgrade) failed " + lastErrorText();
            CloseServiceHandle(svc);
            CloseServiceHandle(scm);
            return false;
        }
    }

    SERVICE_DESCRIPTIONW desc{const_cast<LPWSTR>(kDescription)};
    ChangeServiceConfig2W(svc, SERVICE_CONFIG_DESCRIPTION, &desc);

    // Auto-restart on crash: 5 s, 10 s, then 60 s for any further failure.
    // Reset the failure count after 1 hour of clean uptime.
    SC_ACTION actions[3] = {
        {SC_ACTION_RESTART, 5000},   // 1st failure: restart after 5 s
        {SC_ACTION_RESTART, 10000},  // 2nd failure: restart after 10 s
        {SC_ACTION_RESTART, 60000},  // 3rd+: restart after 60 s
    };
    SERVICE_FAILURE_ACTIONSW fa{};
    fa.dwResetPeriod = 3600; // reset failure count after 1 hour of uptime
    fa.lpCommand     = nullptr;
    fa.lpRebootMsg   = nullptr;
    fa.cActions      = 3;
    fa.lpsaActions   = actions;
    ChangeServiceConfig2W(svc, SERVICE_CONFIG_FAILURE_ACTIONS, &fa);

    // By default SCM only runs failure actions when the process dies without
    // reporting SERVICE_STOPPED. Also trigger them when the service stops
    // itself with a non-zero exit code (e.g. runtime failed to start).
    SERVICE_FAILURE_ACTIONS_FLAG faFlag{TRUE};
    ChangeServiceConfig2W(svc, SERVICE_CONFIG_FAILURE_ACTIONS_FLAG, &faFlag);

    CloseServiceHandle(svc);
    CloseServiceHandle(scm);
    return true;
}

bool uninstallService(std::wstring& error)
{
    SC_HANDLE scm = OpenSCManagerW(nullptr, nullptr, SC_MANAGER_CONNECT);
    if (!scm)
    {
        error = L"OpenSCManager failed " + lastErrorText();
        return false;
    }

    SC_HANDLE svc = OpenServiceW(scm, kServiceName,
                                 SERVICE_STOP | SERVICE_QUERY_STATUS | DELETE);
    if (!svc)
    {
        error = L"OpenService failed " + lastErrorText();
        CloseServiceHandle(scm);
        return false;
    }

    // Best-effort stop before deletion.
    SERVICE_STATUS status{};
    ControlService(svc, SERVICE_CONTROL_STOP, &status);

    const bool ok = DeleteService(svc) != FALSE;
    if (!ok)
        error = L"DeleteService failed " + lastErrorText();

    CloseServiceHandle(svc);
    CloseServiceHandle(scm);
    return ok;
}

} // namespace veyra::service
