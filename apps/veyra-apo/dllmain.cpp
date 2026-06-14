// Veyra APO — Phase 0 no-op stub.
//
// Final form (Phase 2+): an EFX/MFX Audio Processing Object implementing
// IAudioProcessingObject / IAudioProcessingObjectRT, running inside
// audiodg.exe's real-time thread. Until then this DLL does nothing but load
// cleanly, which is all Phase 0's acceptance criteria require.

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

BOOL APIENTRY DllMain(HMODULE /*module*/, DWORD reason, LPVOID /*reserved*/)
{
    switch (reason)
    {
    case DLL_PROCESS_ATTACH:
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}
