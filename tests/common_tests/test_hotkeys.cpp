#include <catch2/catch.hpp>

#include "veyra/Config.h"
#include "veyra/Hotkeys.h"

using namespace veyra;

TEST_CASE("Hotkey: parse + toString round-trips")
{
    auto a = Hotkey::parse("Ctrl+Alt+M");
    REQUIRE(a.has_value());
    CHECK((a->mods & kHotkeyCtrl) != 0);
    CHECK((a->mods & kHotkeyAlt) != 0);
    CHECK((a->mods & kHotkeyShift) == 0);
    CHECK(a->key == 'M');
    CHECK(a->toString() == "Ctrl+Alt+M");

    CHECK(Hotkey::parse("Ctrl+Alt+Up")->toString() == "Ctrl+Alt+Up");
    CHECK(Hotkey::parse("Shift+F5")->toString() == "Shift+F5");
    CHECK(Hotkey::parse("win+space")->toString() == "Win+Space"); // case-insensitive in
}

TEST_CASE("Hotkey: rejects malformed combos")
{
    CHECK(Hotkey::parse("") == std::nullopt);
    CHECK(Hotkey::parse("Ctrl+") == std::nullopt);       // no key
    CHECK(Hotkey::parse("Foo+M") == std::nullopt);       // unknown modifier
    CHECK(Hotkey::parse("Ctrl+Alt+QQ") == std::nullopt); // unknown key
}

TEST_CASE("HotkeysConfig: defaults are present and valid")
{
    const auto d = HotkeysConfig::defaults();
    REQUIRE(d.bindings.size() == 6);
    for (const auto& b : d.bindings)
    {
        CHECK(b.action != HotkeyAction::None);
        CHECK(b.hotkey.valid());
    }
}

TEST_CASE("HotkeyAction: string round-trip")
{
    for (auto a : {HotkeyAction::ToggleMaster, HotkeyAction::VolumeUp, HotkeyAction::NextPreset,
                   HotkeyAction::ToggleMini})
        CHECK(hotkeyActionFromString(toString(a)) == a);
}

TEST_CASE("Config: hotkeys round-trip through JSON")
{
    Config c;
    c.hotkeys.bindings = {
        {HotkeyAction::ToggleMaster, Hotkey::parse("Ctrl+Shift+0").value(), true},
        {HotkeyAction::NextPreset,   Hotkey::parse("Alt+Right").value(),    false},
    };

    auto rt = Config::fromJson(c.toJson());
    REQUIRE(rt.has_value());
    REQUIRE(rt->hotkeys.bindings.size() == 2);
    CHECK(rt->hotkeys.bindings[0].action == HotkeyAction::ToggleMaster);
    CHECK(rt->hotkeys.bindings[0].hotkey.toString() == "Ctrl+Shift+0");
    CHECK(rt->hotkeys.bindings[1].action == HotkeyAction::NextPreset);
    CHECK(rt->hotkeys.bindings[1].enabled == false);
}
