// Veyra UI — Phase 0 no-op stub.
//
// Final form (Phase 4+): a JUCE application drawing its own glass titlebar,
// EQ, visualizers, presets and settings, talking to the service over the
// named-pipe control channel. For Phase 0 it is a GUI-subsystem binary that
// starts and exits 0, proving the target builds and the manifest embeds.

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

int WINAPI WinMain(HINSTANCE /*instance*/, HINSTANCE /*prevInstance*/,
                   LPSTR /*cmdLine*/, int /*showCmd*/)
{
    return 0;
}
