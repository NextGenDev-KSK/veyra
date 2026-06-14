// Veyra Overlay — Phase 0 no-op stub.
//
// Final form (Phase 8+): a WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_TOPMOST
// window rendered with Direct2D, drawing the Sound Tracker HUD from data the
// service pushes over \\.\pipe\veyra-tracker. No D3D hooking — anti-cheat safe.
// For Phase 0 it starts and exits 0.

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

int WINAPI WinMain(HINSTANCE /*instance*/, HINSTANCE /*prevInstance*/,
                   LPSTR /*cmdLine*/, int /*showCmd*/)
{
    return 0;
}
