#include "veyra/Hotkeys.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <sstream>

namespace veyra {

namespace {

// Virtual-key codes we name (no <windows.h> dependency here).
struct Named { int vk; const char* name; };
const std::array<Named, 11> kNamed = {{
    {0x25, "Left"}, {0x26, "Up"}, {0x27, "Right"}, {0x28, "Down"},
    {0x20, "Space"}, {0x24, "Home"}, {0x23, "End"},
    {0x21, "PageUp"}, {0x22, "PageDown"}, {0x2D, "Insert"}, {0x2E, "Delete"},
}};

std::string toUpper(std::string s)
{
    for (char& c : s) c = (char) std::toupper((unsigned char) c);
    return s;
}

std::string keyName(int vk)
{
    if (vk >= 'A' && vk <= 'Z') return std::string(1, (char) vk);
    if (vk >= '0' && vk <= '9') return std::string(1, (char) vk);
    if (vk >= 0x70 && vk <= 0x87) return "F" + std::to_string(vk - 0x6F); // F1..F24
    for (const auto& n : kNamed) if (n.vk == vk) return n.name;
    return {};
}

int keyFromName(const std::string& tokenIn)
{
    const std::string t = toUpper(tokenIn);
    if (t.size() == 1)
    {
        const char c = t[0];
        if ((c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9')) return (int) c;
    }
    if (t.size() >= 2 && t[0] == 'F')
    {
        bool digits = true;
        for (size_t i = 1; i < t.size(); ++i) if (!std::isdigit((unsigned char) t[i])) digits = false;
        if (digits)
        {
            const int n = std::stoi(t.substr(1));
            if (n >= 1 && n <= 24) return 0x6F + n;
        }
    }
    for (const auto& n : kNamed) if (toUpper(n.name) == t) return n.vk;
    return 0;
}

} // namespace

std::string Hotkey::toString() const
{
    if (!valid()) return {};
    std::string s;
    if (mods & kHotkeyCtrl)  s += "Ctrl+";
    if (mods & kHotkeyAlt)   s += "Alt+";
    if (mods & kHotkeyShift) s += "Shift+";
    if (mods & kHotkeyWin)   s += "Win+";
    s += keyName(key);
    return s;
}

std::optional<Hotkey> Hotkey::parse(const std::string& text)
{
    if (text.empty()) return std::nullopt;

    std::vector<std::string> tokens;
    std::stringstream ss(text);
    std::string tok;
    while (std::getline(ss, tok, '+'))
    {
        // trim spaces
        const auto a = tok.find_first_not_of(" \t");
        const auto b = tok.find_last_not_of(" \t");
        if (a != std::string::npos) tokens.push_back(tok.substr(a, b - a + 1));
    }
    if (tokens.empty()) return std::nullopt;

    Hotkey h;
    for (size_t i = 0; i + 1 < tokens.size(); ++i)
    {
        const std::string m = toUpper(tokens[i]);
        if (m == "CTRL" || m == "CONTROL") h.mods |= kHotkeyCtrl;
        else if (m == "ALT")               h.mods |= kHotkeyAlt;
        else if (m == "SHIFT")             h.mods |= kHotkeyShift;
        else if (m == "WIN" || m == "META" || m == "CMD") h.mods |= kHotkeyWin;
        else return std::nullopt; // unknown modifier
    }
    h.key = keyFromName(tokens.back());
    if (h.key == 0) return std::nullopt;
    return h;
}

std::string toString(HotkeyAction action)
{
    switch (action)
    {
    case HotkeyAction::ToggleMaster: return "toggleMaster";
    case HotkeyAction::VolumeUp:     return "volumeUp";
    case HotkeyAction::VolumeDown:   return "volumeDown";
    case HotkeyAction::NextPreset:   return "nextPreset";
    case HotkeyAction::PrevPreset:   return "prevPreset";
    case HotkeyAction::ToggleMini:   return "toggleMini";
    default:                         return "none";
    }
}

HotkeyAction hotkeyActionFromString(const std::string& s)
{
    if (s == "toggleMaster") return HotkeyAction::ToggleMaster;
    if (s == "volumeUp")     return HotkeyAction::VolumeUp;
    if (s == "volumeDown")   return HotkeyAction::VolumeDown;
    if (s == "nextPreset")   return HotkeyAction::NextPreset;
    if (s == "prevPreset")   return HotkeyAction::PrevPreset;
    if (s == "toggleMini")   return HotkeyAction::ToggleMini;
    return HotkeyAction::None;
}

HotkeysConfig HotkeysConfig::defaults()
{
    auto mk = [](HotkeyAction a, const char* combo) {
        HotkeyBinding b;
        b.action = a;
        b.hotkey = Hotkey::parse(combo).value_or(Hotkey{});
        b.enabled = true;
        return b;
    };
    HotkeysConfig c;
    c.bindings = {
        mk(HotkeyAction::ToggleMaster, "Ctrl+Alt+M"),
        mk(HotkeyAction::VolumeUp,     "Ctrl+Alt+Up"),
        mk(HotkeyAction::VolumeDown,   "Ctrl+Alt+Down"),
        mk(HotkeyAction::NextPreset,   "Ctrl+Alt+Right"),
        mk(HotkeyAction::PrevPreset,   "Ctrl+Alt+Left"),
        mk(HotkeyAction::ToggleMini,   "Ctrl+Alt+N"),
    };
    return c;
}

} // namespace veyra
