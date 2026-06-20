#pragma once

// Registers system-wide hotkeys (RegisterHotKey) on a dedicated thread and
// dispatches the bound HotkeyAction to a callback marshalled onto the JUCE
// message thread. Runtime/compile-verified — global hotkeys can't be exercised
// in CI.

#include <atomic>
#include <functional>
#include <thread>

#include "veyra/Hotkeys.h"

namespace veyra::ui {

class HotkeyManager {
public:
    using Callback = std::function<void(veyra::HotkeyAction)>;

    ~HotkeyManager();

    void start(const veyra::HotkeysConfig& config, Callback onAction);
    void stop();

private:
    void run(veyra::HotkeysConfig config);

    std::thread             thread_;
    std::atomic<bool>       running_{false};
    std::atomic<unsigned long> threadId_{0};
    Callback                cb_;
};

} // namespace veyra::ui
