#include <catch2/catch.hpp>

#include "veyra/Localization.h"

using namespace veyra;

TEST_CASE("Localization: English base resolves known keys")
{
    Localization l;
    CHECK(l.tr("nav.home") == "Home");
    CHECK(l.tr("settings.loudness") == "Loudness");
}

TEST_CASE("Localization: unknown keys surface the key itself")
{
    Localization l;
    CHECK(l.tr("does.not.exist") == "does.not.exist");
}

TEST_CASE("Localization: overlay overrides, untranslated falls back to English")
{
    Localization l;
    l.setOverlayJson(R"({ "nav.home": "Inicio", "common.save": "Guardar" })");

    CHECK(l.tr("nav.home") == "Inicio");      // overridden
    CHECK(l.tr("common.save") == "Guardar");  // overridden
    CHECK(l.tr("nav.settings") == "Settings"); // not in overlay -> English base
    CHECK(l.tr("nope") == "nope");             // still surfaces unknown keys
}

TEST_CASE("Localization: malformed overlay JSON is ignored (stays English)")
{
    Localization l;
    l.setOverlayJson("not json at all");
    CHECK(l.tr("nav.home") == "Home");

    l.setOverlayJson(R"(["array","not","object"])");
    CHECK(l.tr("nav.home") == "Home");
}

TEST_CASE("Localization: clearOverlay reverts to English")
{
    Localization l;
    l.setOverlayJson(R"({ "nav.home": "Inicio" })");
    REQUIRE(l.tr("nav.home") == "Inicio");
    l.clearOverlay();
    CHECK(l.tr("nav.home") == "Home");
}
