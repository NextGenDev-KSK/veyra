// Veyra Sounds UI — Phase 1 shell.
//
// A minimal native window that proves the UI <-> service IPC backbone: it shows
// the live connection status ("Service connected · v<version>", or an animated
// reconnect indicator) sourced from the background ServiceClient. The full JUCE
// application replaces this shell in Phase 4; the ServiceClient carries over.

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

#include <string>

#include "ServiceClient.h"

namespace {

constexpr wchar_t kWindowClass[] = L"VeyraUiMainWindow";
constexpr UINT    WM_APP_STATUS  = WM_APP + 1;
constexpr UINT    kSpinnerTimerId = 1;

veyra::ui::ServiceClient g_client;
int                      g_spinnerPhase = 0;

std::wstring statusLine()
{
    const auto status = g_client.status();
    const std::wstring dots(static_cast<size_t>(g_spinnerPhase % 4), L'.');

    switch (status.state)
    {
    case veyra::ui::ConnectionState::Connected:
        return L"Service connected \x00B7 v" + status.serviceVersion;
    case veyra::ui::ConnectionState::Reconnecting:
        return L"Reconnecting" + dots;
    case veyra::ui::ConnectionState::Connecting:
    default:
        return L"Connecting to Veyra service" + dots;
    }
}

void paint(HWND hwnd)
{
    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(hwnd, &ps);

    RECT rc;
    GetClientRect(hwnd, &rc);

    // Dark background to match the eventual glass aesthetic.
    HBRUSH bg = CreateSolidBrush(RGB(18, 18, 22));
    FillRect(hdc, &rc, bg);
    DeleteObject(bg);

    SetBkMode(hdc, TRANSPARENT);

    // Title.
    HFONT titleFont = CreateFontW(-34, 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE,
                                  DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
                                  CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
                                  DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
    HFONT old = static_cast<HFONT>(SelectObject(hdc, titleFont));
    SetTextColor(hdc, RGB(245, 245, 250));
    RECT titleRc = rc;
    titleRc.bottom = rc.bottom / 2;
    DrawTextW(hdc, L"Veyra Sounds", -1, &titleRc,
              DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    SelectObject(hdc, old);
    DeleteObject(titleFont);

    // Status line.
    const bool connected =
        g_client.status().state == veyra::ui::ConnectionState::Connected;
    HFONT statusFont = CreateFontW(-18, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                                   DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
                                   CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
                                   DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
    old = static_cast<HFONT>(SelectObject(hdc, statusFont));
    SetTextColor(hdc, connected ? RGB(120, 220, 150) : RGB(180, 180, 190));
    RECT statusRc = rc;
    statusRc.top = rc.bottom / 2;
    const std::wstring line = statusLine();
    DrawTextW(hdc, line.c_str(), -1, &statusRc,
              DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    SelectObject(hdc, old);
    DeleteObject(statusFont);

    EndPaint(hwnd, &ps);
}

LRESULT CALLBACK wndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_CREATE:
        g_client.start(hwnd, WM_APP_STATUS);
        SetTimer(hwnd, kSpinnerTimerId, 400, nullptr);
        return 0;

    case WM_TIMER:
        if (wParam == kSpinnerTimerId)
        {
            // Animate the spinner only while we're not connected.
            if (g_client.status().state != veyra::ui::ConnectionState::Connected)
            {
                ++g_spinnerPhase;
                InvalidateRect(hwnd, nullptr, FALSE);
            }
        }
        return 0;

    case WM_APP_STATUS:
        InvalidateRect(hwnd, nullptr, FALSE);
        return 0;

    case WM_ERASEBKGND:
        return 1; // handled in WM_PAINT to avoid flicker

    case WM_PAINT:
        paint(hwnd);
        return 0;

    case WM_DESTROY:
        KillTimer(hwnd, kSpinnerTimerId);
        g_client.stop();
        PostQuitMessage(0);
        return 0;

    default:
        return DefWindowProcW(hwnd, msg, wParam, lParam);
    }
}

} // namespace

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR, int showCmd)
{
    WNDCLASSEXW wc{};
    wc.cbSize = sizeof(wc);
    wc.lpfnWndProc = wndProc;
    wc.hInstance = instance;
    wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    wc.lpszClassName = kWindowClass;
    RegisterClassExW(&wc);

    const int width = 560, height = 280;
    const int x = (GetSystemMetrics(SM_CXSCREEN) - width) / 2;
    const int y = (GetSystemMetrics(SM_CYSCREEN) - height) / 2;

    HWND hwnd = CreateWindowExW(
        0, kWindowClass, L"Veyra Sounds",
        WS_OVERLAPPEDWINDOW & ~(WS_MAXIMIZEBOX | WS_THICKFRAME),
        x, y, width, height, nullptr, nullptr, instance, nullptr);
    if (!hwnd)
        return 1;

    ShowWindow(hwnd, showCmd);
    UpdateWindow(hwnd);

    MSG msg;
    while (GetMessageW(&msg, nullptr, 0, 0) > 0)
    {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
    return static_cast<int>(msg.wParam);
}
