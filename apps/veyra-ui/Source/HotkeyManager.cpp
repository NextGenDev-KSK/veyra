#include "HotkeyManager.h"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

#include <vector>

#include "VeyraGui.h" // juce::MessageManager

namespace veyra::ui {

namespace {
UINT toWin32Mods(int mods)
{
    UINT m = 0;
    if (mods & veyra::kHotkeyCtrl)  m |= MOD_CONTROL;
    if (mods & veyra::kHotkeyAlt)   m |= MOD_ALT;
    if (mods & veyra::kHotkeyShift) m |= MOD_SHIFT;
    if (mods & veyra::kHotkeyWin)   m |= MOD_WIN;
    return m | MOD_NOREPEAT;
}
} // namespace

HotkeyManager::~HotkeyManager()
{
    stop();
}

void HotkeyManager::start(const veyra::HotkeysConfig& config, Callback onAction)
{
    stop();
    cb_ = std::move(onAction);
    running_.store(true);
    thread_ = std::thread(&HotkeyManager::run, this, config);
}

void HotkeyManager::stop()
{
    if (!running_.exchange(false))
        return;
    // Wake the thread's GetMessage loop.
    for (int i = 0; i < 200 && threadId_.load() == 0; ++i)
        Sleep(1);
    if (const auto tid = threadId_.load())
        PostThreadMessageW(tid, WM_QUIT, 0, 0);
    if (thread_.joinable())
        thread_.join();
    threadId_.store(0);
}

void HotkeyManager::run(veyra::HotkeysConfig config)
{
    threadId_.store(GetCurrentThreadId());

    // Force the queue to exist before anyone PostThreadMessage's us.
    MSG probe;
    PeekMessageW(&probe, nullptr, WM_USER, WM_USER, PM_NOREMOVE);

    std::vector<veyra::HotkeyAction> byId; // index = hotkey id - 1
    int id = 1;
    for (const auto& b : config.bindings)
    {
        if (!b.enabled || !b.hotkey.valid() || b.action == veyra::HotkeyAction::None)
            continue;
        if (RegisterHotKey(nullptr, id, toWin32Mods(b.hotkey.mods), (UINT) b.hotkey.key))
        {
            byId.push_back(b.action);
            ++id;
        }
    }

    MSG msg;
    while (running_.load() && GetMessageW(&msg, nullptr, 0, 0) > 0)
    {
        if (msg.message == WM_HOTKEY)
        {
            const int idx = (int) msg.wParam - 1;
            if (idx >= 0 && idx < (int) byId.size())
            {
                const auto action = byId[(size_t) idx];
                Callback cb = cb_;
                juce::MessageManager::callAsync([cb, action] { if (cb) cb(action); });
            }
        }
    }

    for (int i = 1; i < id; ++i)
        UnregisterHotKey(nullptr, i);
}

} // namespace veyra::ui
