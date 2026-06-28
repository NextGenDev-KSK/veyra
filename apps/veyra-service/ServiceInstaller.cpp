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
        error = L"CreateService failed " + lastErrorText();
        CloseServiceHandle(scm);
        return false;
    }

    SERVICE_DESCRIPTIONW desc{const_cast<LPWSTR>(kDescription)};
    ChangeServiceConfig2W(svc, SERVICE_CONFIG_DESCRIPTION, &desc);

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
