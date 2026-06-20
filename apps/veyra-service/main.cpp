// Veyra Audio Service — entry point and launch-mode dispatch.
//
//   (no args)      run under the SCM (how Windows starts the service)
//   --console      run the same runtime in the foreground for debugging
//   --install      register the auto-start LocalSystem service
//   --uninstall    stop + remove the service
//
// Installing/uninstalling requires an elevated (admin) prompt.

#include "CrashHandler.h"
#include "ServiceInstaller.h"
#include "ServiceMain.h"
#include "ServiceRuntime.h"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

#include <cstdio>
#include <string>

#include "veyra/Paths.h"
#include "veyra/version.h"

namespace {

HANDLE g_consoleStop = nullptr;

BOOL WINAPI consoleCtrlHandler(DWORD type)
{
    switch (type)
    {
    case CTRL_C_EVENT:
    case CTRL_BREAK_EVENT:
    case CTRL_CLOSE_EVENT:
    case CTRL_SHUTDOWN_EVENT:
        if (g_consoleStop)
            SetEvent(g_consoleStop);
        return TRUE;
    default:
        return FALSE;
    }
}

void printUsage()
{
    wprintf(L"Veyra Audio Service v%hs\n\n", veyra::kVersionString);
    wprintf(L"Usage:\n");
    wprintf(L"  veyra-service                run under the Service Control Manager\n");
    wprintf(L"  veyra-service --console      run in the foreground (Ctrl+C to stop)\n");
    wprintf(L"  veyra-service --install      install the auto-start service (admin)\n");
    wprintf(L"  veyra-service --uninstall    remove the service (admin)\n");
}

int runConsole()
{
    wprintf(L"Veyra Audio Service v%hs (console mode)\n", veyra::kVersionString);

    veyra::service::ServiceRuntime runtime(/*consoleLogging=*/true);
    if (!runtime.start())
    {
        wprintf(L"Failed to start runtime. See the log for details.\n");
        return 1;
    }

    g_consoleStop = CreateEventW(nullptr, TRUE, FALSE, nullptr);
    SetConsoleCtrlHandler(consoleCtrlHandler, TRUE);

    wprintf(L"Running. Press Ctrl+C to stop.\n");
    WaitForSingleObject(g_consoleStop, INFINITE);

    runtime.stop();
    if (g_consoleStop)
        CloseHandle(g_consoleStop);
    return 0;
}

} // namespace

int wmain(int argc, wchar_t** argv)
{
    // Capture unhandled crashes (report + minidump) before anything else runs.
    veyra::service::CrashHandler::install("veyra-service", veyra::kVersionString,
                                          veyra::kGitCommit, veyra::paths::crashesDir());

    if (argc >= 2)
    {
        const std::wstring cmd = argv[1];

        if (cmd == L"--console")
            return runConsole();

        if (cmd == L"--install")
        {
            std::wstring error;
            if (veyra::service::installService(error))
            {
                wprintf(L"Service installed.\n");
                return 0;
            }
            wprintf(L"Install failed: %ls\n", error.c_str());
            return 1;
        }

        if (cmd == L"--uninstall")
        {
            std::wstring error;
            if (veyra::service::uninstallService(error))
            {
                wprintf(L"Service removed.\n");
                return 0;
            }
            wprintf(L"Uninstall failed: %ls\n", error.c_str());
            return 1;
        }

        if (cmd == L"--help" || cmd == L"-h" || cmd == L"/?")
        {
            printUsage();
            return 0;
        }

        wprintf(L"Unknown option: %ls\n\n", cmd.c_str());
        printUsage();
        return 1;
    }

    // No arguments: assume the SCM launched us.
    return veyra::service::runAsService();
}
