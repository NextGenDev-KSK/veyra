#include "CrashHandler.h"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <dbghelp.h>

#include <cstdio>

#include "veyra/CrashReport.h"

namespace veyra::service {

namespace {

std::string             g_process;
std::string             g_version;
std::string             g_commit;
std::filesystem::path   g_crashesDir;
LPTOP_LEVEL_EXCEPTION_FILTER g_previous = nullptr;

std::string isoNow()
{
    SYSTEMTIME st;
    GetSystemTime(&st);
    char buf[40];
    std::snprintf(buf, sizeof(buf), "%04u-%02u-%02uT%02u:%02u:%02uZ",
                  st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);
    return buf;
}

std::string moduleAt(void* address)
{
    HMODULE mod = nullptr;
    if (!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
                                GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                            reinterpret_cast<LPCWSTR>(address), &mod) || !mod)
        return {};
    wchar_t path[MAX_PATH] = {};
    if (GetModuleFileNameW(mod, path, MAX_PATH) == 0)
        return {};
    std::wstring w(path);
    const auto slash = w.find_last_of(L"\\/");
    const std::wstring base = (slash == std::wstring::npos) ? w : w.substr(slash + 1);
    const int n = WideCharToMultiByte(CP_UTF8, 0, base.data(), (int) base.size(), nullptr, 0, nullptr, nullptr);
    std::string out((size_t) (n > 0 ? n : 0), '\0');
    if (n > 0)
        WideCharToMultiByte(CP_UTF8, 0, base.data(), (int) base.size(), out.data(), n, nullptr, nullptr);
    return out;
}

void writeMinidump(EXCEPTION_POINTERS* ep, const std::string& stamp)
{
    std::wstring file = (g_crashesDir / ("crash-" + stamp + ".dmp")).wstring();
    HANDLE h = CreateFileW(file.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS,
                           FILE_ATTRIBUTE_NORMAL, nullptr);
    if (h == INVALID_HANDLE_VALUE)
        return;

    MINIDUMP_EXCEPTION_INFORMATION mei{};
    mei.ThreadId = GetCurrentThreadId();
    mei.ExceptionPointers = ep;
    mei.ClientPointers = FALSE;
    MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(), h,
                      MiniDumpNormal, ep ? &mei : nullptr, nullptr, nullptr);
    CloseHandle(h);
}

LONG WINAPI handler(EXCEPTION_POINTERS* ep)
{
    const std::string stamp = []() {
        std::string s = isoNow();
        for (char& c : s)
            if (c == ':') c = '-';
        return s;
    }();

    CrashReport r;
    r.version = g_version;
    r.gitCommit = g_commit;
    r.timestamp = isoNow();
    r.process = g_process;
    if (ep && ep->ExceptionRecord)
    {
        r.exceptionCode = ep->ExceptionRecord->ExceptionCode;
        r.address = reinterpret_cast<uint64_t>(ep->ExceptionRecord->ExceptionAddress);
        r.module = moduleAt(ep->ExceptionRecord->ExceptionAddress);
    }
    char msg[96];
    std::snprintf(msg, sizeof(msg), "Unhandled exception 0x%08X in %s",
                  r.exceptionCode, r.module.empty() ? "?" : r.module.c_str());
    r.message = msg;

    writeCrashReport(r, g_crashesDir);
    writeMinidump(ep, stamp);

    return g_previous ? g_previous(ep) : EXCEPTION_EXECUTE_HANDLER;
}

} // namespace

void CrashHandler::install(std::string process, std::string version, std::string gitCommit,
                           std::filesystem::path crashesDir)
{
    g_process = std::move(process);
    g_version = std::move(version);
    g_commit = std::move(gitCommit);
    g_crashesDir = std::move(crashesDir);
    std::error_code ec;
    std::filesystem::create_directories(g_crashesDir, ec);
    g_previous = SetUnhandledExceptionFilter(handler);
}

} // namespace veyra::service
