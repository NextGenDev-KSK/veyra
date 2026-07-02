// Veyra Overlay — the Sound Tracker HUD (Phase 8).
//
// A layered, click-through, always-on-top tool window rendered with GDI+. It
// reads detections from the VeyraTrackerData shared block (written by the
// service's loopback Sound Tracker) and draws a radar. It never hooks a graphics
// API or injects into any process — it is an ordinary top-level window composited
// by the desktop, which keeps it anti-cheat safe. Visibility + radar style follow
// the Gamer Mode settings polled from the live config.

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <objidl.h> // IStream / PROPID — required by the GDI+ headers

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <vector>

// The project defines NOMINMAX globally, but the GDI+ headers use unqualified
// min/max; bring them into the Gdiplus namespace before including it.
namespace Gdiplus { using std::min; using std::max; }
#include <gdiplus.h>

#include "veyra/Config.h"
#include "veyra/Paths.h"
#include "veyra/ipc/SharedMemory.h"
#include "veyra/ipc/TrackerData.h"

#pragma comment(lib, "gdiplus.lib")

using namespace Gdiplus;

namespace {

constexpr int   kW = 340;
constexpr int   kH = 360;
constexpr float kFrameDt = 0.033f; // ~30 fps
constexpr float kBlipLife = 1.6f;  // seconds

struct Blip {
    float azimuth = 0.0f;
    int   type = 0;
    float intensity = 0.0f;
    float age = 0.0f;
};

HWND                                g_hwnd = nullptr;
HBITMAP                             g_dib = nullptr;
void*                               g_bits = nullptr;
HDC                                 g_memDC = nullptr;
veyra::ipc::SharedMemoryRegion      g_region;
const veyra::ipc::VeyraTrackerData* g_data = nullptr;
uint64_t                            g_seen = 0;
std::vector<Blip>                   g_blips;
float                               g_sweep = 0.0f;
int                                 g_pollCounter = 0;
veyra::GamerModeConfig              g_gm;

Color blipColour(int type, BYTE alpha)
{
    switch (static_cast<veyra::ipc::TrackerEventType>(type))
    {
    case veyra::ipc::TrackerEventType::Footstep: return Color(alpha, 60, 200, 255); // cyan
    case veyra::ipc::TrackerEventType::Gunshot:  return Color(alpha, 255, 80, 60);  // red
    case veyra::ipc::TrackerEventType::Voice:    return Color(alpha, 255, 210, 80); // amber
    default:                                     return Color(alpha, 200, 200, 210);
    }
}

bool typeEnabled(int type)
{
    switch (static_cast<veyra::ipc::TrackerEventType>(type))
    {
    case veyra::ipc::TrackerEventType::Footstep: return g_gm.footsteps;
    case veyra::ipc::TrackerEventType::Gunshot:  return g_gm.gunshots;
    case veyra::ipc::TrackerEventType::Voice:    return g_gm.voices;
    default:                                     return false;
    }
}

void pollConfig()
{
    if (auto c = veyra::Config::load(veyra::paths::configFile()))
        g_gm = c->gamerMode;
}

void drainEvents()
{
    if (!g_data)
        return;
    veyra::ipc::TrackerEventRecord recs[veyra::ipc::VeyraTrackerData::kCapacity];
    const int n = veyra::ipc::readTrackerEvents(g_data, g_seen, recs,
                                                veyra::ipc::VeyraTrackerData::kCapacity);
    for (int i = 0; i < n; ++i)
        if (typeEnabled(recs[i].type))
            g_blips.push_back({recs[i].azimuthDeg, recs[i].type, recs[i].intensity, 0.0f});
}

void ageBlips()
{
    for (auto& b : g_blips)
        b.age += kFrameDt;
    g_blips.erase(std::remove_if(g_blips.begin(), g_blips.end(),
                                 [](const Blip& b) { return b.age >= kBlipLife; }),
                  g_blips.end());
}

void drawRadar(Graphics& g)
{
    g.SetSmoothingMode(SmoothingModeAntiAlias);
    g.Clear(Color(0, 0, 0, 0)); // fully transparent

    const float cx = kW * 0.5f;
    const float cy = kH * 0.5f;
    const float R = 150.0f;
    const bool rich = g_gm.radarMode == 1;
    const bool compass = g_gm.radarMode == 2;

    if (compass)
    {
        // A horizontal bearing strip: L .. C .. R with blips along it.
        const float y = 40.0f, x0 = 30.0f, x1 = kW - 30.0f;
        Pen axis(Color(120, 255, 255, 255), 2.0f);
        g.DrawLine(&axis, x0, y, x1, y);
        SolidBrush tick(Color(150, 200, 200, 210));
        for (int m = -90; m <= 90; m += 45)
        {
            const float t = (m + 90) / 180.0f;
            const float x = x0 + t * (x1 - x0);
            g.FillRectangle(&tick, x - 1.0f, y - 6.0f, 2.0f, 12.0f);
        }
        for (const auto& b : g_blips)
        {
            const BYTE a = (BYTE) (255 * (1.0f - b.age / kBlipLife));
            const float t = (std::min(90.0f, std::max(-90.0f, b.azimuth)) + 90) / 180.0f;
            const float x = x0 + t * (x1 - x0);
            const float sz = 6.0f + 10.0f * std::min(1.0f, b.intensity);
            SolidBrush br(blipColour(b.type, a));
            g.FillEllipse(&br, x - sz * 0.5f, y - sz * 0.5f, sz, sz);
        }
        return;
    }

    // Circular radar (competitive + rich).
    SolidBrush disc(Color((BYTE) (rich ? 90 : 60), 8, 10, 18));
    g.FillEllipse(&disc, cx - R, cy - R, R * 2, R * 2);

    Pen ring(Color(140, 90, 110, 160), 1.5f);
    g.DrawEllipse(&ring, cx - R, cy - R, R * 2, R * 2);
    if (rich)
    {
        Pen faint(Color(70, 90, 110, 160), 1.0f);
        for (float rr : {R * 0.66f, R * 0.33f})
            g.DrawEllipse(&faint, cx - rr, cy - rr, rr * 2, rr * 2);
        g.DrawLine(&faint, cx, cy - R, cx, cy + R);
        g.DrawLine(&faint, cx - R, cy, cx + R, cy);

        // Sweep line.
        g_sweep += 1.6f * kFrameDt;
        const float sx = cx + R * std::sin(g_sweep);
        const float sy = cy - R * std::cos(g_sweep);
        Pen sweepPen(Color(120, 80, 220, 160), 2.0f);
        g.DrawLine(&sweepPen, cx, cy, sx, sy);
    }

    // Player marker.
    SolidBrush you(Color(230, 124, 92, 255));
    g.FillEllipse(&you, cx - 4.0f, cy - 4.0f, 8.0f, 8.0f);

    // Blips on the front semicircle (azimuth: 0 = up, +90 = right, -90 = left).
    for (const auto& b : g_blips)
    {
        const float life = 1.0f - b.age / kBlipLife;
        const BYTE a = (BYTE) (255 * life);
        const float rad = b.azimuth * 3.14159265f / 180.0f;
        const float dist = R * (0.45f + 0.4f * std::min(1.0f, b.intensity));
        const float x = cx + dist * std::sin(rad);
        const float y = cy - dist * std::cos(rad);
        const float sz = 7.0f + 11.0f * std::min(1.0f, b.intensity);

        SolidBrush glow(blipColour(b.type, (BYTE) (a * 0.35f)));
        g.FillEllipse(&glow, x - sz, y - sz, sz * 2, sz * 2);
        SolidBrush dot(blipColour(b.type, a));
        g.FillEllipse(&dot, x - sz * 0.5f, y - sz * 0.5f, sz, sz);
    }
}

void premultiply()
{
    auto* px = static_cast<uint8_t*>(g_bits);
    if (!px)
        return;
    const int count = kW * kH;
    for (int i = 0; i < count; ++i)
    {
        uint8_t* p = px + i * 4; // BGRA
        const uint32_t a = p[3];
        p[0] = (uint8_t) (p[0] * a / 255);
        p[1] = (uint8_t) (p[1] * a / 255);
        p[2] = (uint8_t) (p[2] * a / 255);
    }
}

void render()
{
    // Poll config.json every 300 frames (~10 s at 30 fps). Full JSON parse on
    // every 15 frames (~0.5 s) stalls the render loop on HDDs and network AppData.
    if (++g_pollCounter % 300 == 0)
    {
        pollConfig();

        // If the service wasn't up when we launched, keep retrying the tracker
        // block on the same cadence instead of staying dead until relaunch.
        if (!g_data &&
            g_region.open(veyra::ipc::kSharedTrackerName, sizeof(veyra::ipc::VeyraTrackerData)))
        {
            g_data = static_cast<const veyra::ipc::VeyraTrackerData*>(g_region.data());
            if (g_data)
                g_seen = g_data->writeCount.load(std::memory_order_acquire);
        }
    }

    if (!g_gm.enabled)
    {
        ShowWindow(g_hwnd, SW_HIDE);
        g_blips.clear();
        return;
    }
    if (!IsWindowVisible(g_hwnd))
        ShowWindow(g_hwnd, SW_SHOWNOACTIVATE);

    drainEvents();
    ageBlips();

    Graphics g(g_memDC);
    drawRadar(g);
    premultiply();

    const int screenW = GetSystemMetrics(SM_CXSCREEN);
    POINT ptPos = {(screenW - kW) / 2, 60};
    SIZE  size = {kW, kH};
    POINT ptSrc = {0, 0};
    BLENDFUNCTION blend = {AC_SRC_OVER, 0, 255, AC_SRC_ALPHA};
    HDC screenDC = GetDC(nullptr);
    UpdateLayeredWindow(g_hwnd, screenDC, &ptPos, &size, g_memDC, &ptSrc, 0, &blend, ULW_ALPHA);
    ReleaseDC(nullptr, screenDC);
}

LRESULT CALLBACK wndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_TIMER:
        render();
        return 0;
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    default:
        return DefWindowProcW(hwnd, msg, wParam, lParam);
    }
}

} // namespace

int WINAPI WinMain(HINSTANCE instance, HINSTANCE, LPSTR, int)
{
    ULONG_PTR gdiplusToken = 0;
    GdiplusStartupInput gdiplusStartupInput;
    GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, nullptr);

    WNDCLASSEXW wc = {};
    wc.cbSize = sizeof(wc);
    wc.lpfnWndProc = wndProc;
    wc.hInstance = instance;
    wc.lpszClassName = L"VeyraOverlayWindow";
    RegisterClassExW(&wc);

    g_hwnd = CreateWindowExW(
        WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE,
        wc.lpszClassName, L"Veyra Overlay", WS_POPUP,
        0, 0, kW, kH, nullptr, nullptr, instance, nullptr);
    if (!g_hwnd)
        return 1;

    // Back buffer: a top-down 32bpp DIB the GDI+ surface draws into.
    BITMAPINFO bmi = {};
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = kW;
    bmi.bmiHeader.biHeight = -kH; // top-down
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;
    HDC screenDC = GetDC(nullptr);
    g_dib = CreateDIBSection(screenDC, &bmi, DIB_RGB_COLORS, &g_bits, nullptr, 0);
    g_memDC = CreateCompatibleDC(screenDC);
    SelectObject(g_memDC, g_dib);
    ReleaseDC(nullptr, screenDC);

    // Map the tracker channel (read-only view of the service's block).
    if (g_region.open(veyra::ipc::kSharedTrackerName, sizeof(veyra::ipc::VeyraTrackerData)))
    {
        g_data = static_cast<const veyra::ipc::VeyraTrackerData*>(g_region.data());
        if (g_data)
            g_seen = g_data->writeCount.load(std::memory_order_acquire);
    }

    pollConfig();
    SetTimer(g_hwnd, 1, 33, nullptr);

    MSG msg;
    while (GetMessageW(&msg, nullptr, 0, 0) > 0)
    {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    KillTimer(g_hwnd, 1);
    if (g_memDC) DeleteDC(g_memDC);
    if (g_dib) DeleteObject(g_dib);
    GdiplusShutdown(gdiplusToken);
    return 0;
}
