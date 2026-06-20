#pragma once

// Global-hotkey model: an action + a key combo (modifiers + virtual-key code),
// with human-readable parse/format ("Ctrl+Alt+M") and a default binding set. The
// UI's HotkeyManager registers these system-wide (RegisterHotKey) and dispatches
// the action; this header is pure (no Win32 / nlohmann) so it's unit-testable.

#include <optional>
#include <string>
#include <vector>

namespace veyra {

// Modifier bitmask (independent of Win32's MOD_* ordering; the manager maps it).
enum HotkeyMod { kHotkeyCtrl = 1, kHotkeyAlt = 2, kHotkeyShift = 4, kHotkeyWin = 8 };

enum class HotkeyAction {
    None,
    ToggleMaster,
    VolumeUp,
    VolumeDown,
    NextPreset,
    PrevPreset,
    ToggleMini,
};

struct Hotkey {
    int mods = 0; // HotkeyMod bitmask
    int key = 0;  // virtual-key code; 0 = unbound

    bool valid() const noexcept { return key != 0; }

    std::string toString() const;                       // "Ctrl+Alt+M" / "" if unbound
    static std::optional<Hotkey> parse(const std::string& text);
};

struct HotkeyBinding {
    HotkeyAction action = HotkeyAction::None;
    Hotkey       hotkey;
    bool         enabled = true;
};

struct HotkeysConfig {
    std::vector<HotkeyBinding> bindings;
    static HotkeysConfig defaults();
};

std::string  toString(HotkeyAction action);
HotkeyAction hotkeyActionFromString(const std::string& s);

} // namespace veyra
