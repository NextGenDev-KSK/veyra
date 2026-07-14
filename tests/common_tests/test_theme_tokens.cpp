#include <catch2/catch.hpp>

#include "veyra/Config.h"
#include "veyra/ThemeTokens.h"

using namespace veyra::theme;

namespace {

// Sample anchors used by the per-theme load test (from the port spec).
CustomAnchors sampleAnchors()
{
    CustomAnchors a;
    a.accent     = parseHexColor("#FF6B35");
    a.background = parseHexColor("#101418");
    return a;
}

// Every token a palette must define; kUnset (zero alpha) fails the theme.
void requireAllTokensDefined(const ThemeTokens& t, const std::string& id)
{
    INFO("theme: " << id);
    const Argb all[] = {
        t.surfaceApp, t.surfacePanel, t.surfaceRaised, t.surfaceSunken, t.surfaceNav,
        t.border, t.borderStrong,
        t.accent, t.accentBright, t.accentDeep,
        t.success, t.warning, t.danger,
        t.meterLow, t.meterMid, t.meterHigh, t.meterTrack,
        t.textPrimary, t.textSecondary, t.textMuted, t.textOnAccent, t.textTitle,
        t.borderGlow(), t.accentWash(), ThemeTokens::scrim(),
    };
    for (const Argb c : all)
    {
        CHECK(c != kUnset);
        CHECK((c >> 24) != 0); // a defined colour always carries alpha
    }
}

ThemeTokens tokensFor(const std::string& id)
{
    return id == kCustomThemeId ? deriveCustom(sampleAnchors()) : themeTokens(id);
}

} // namespace

// --- verification 1: load test once per theme (all curated + Custom) ----------

TEST_CASE("Themes: every palette in the catalog defines every token")
{
    const auto& catalog = themeCatalog();
    REQUIRE(catalog.size() == 15); // 14 curated + Custom; no themes beyond the list

    for (const auto& info : catalog)
        requireAllTokensDefined(tokensFor(info.id), info.id);
}

TEST_CASE("Themes: gallery order is Violet first, Custom last")
{
    const auto& catalog = themeCatalog();
    CHECK(catalog.front().id == "violet");
    CHECK(catalog.front().id == kDefaultThemeId);
    CHECK(catalog.back().id == kCustomThemeId);
}

TEST_CASE("Themes: canonical values are exact (community palettes untouched)")
{
    const auto violet = themeTokens("violet");
    CHECK(violet.accent == 0xFF8B5CF6);
    CHECK(violet.accentBright == 0xFFA78BFA);
    CHECK(violet.accentDeep == 0xFF7C3AED);
    CHECK(violet.surfaceApp == 0xFF0C0814);
    CHECK(violet.meterLow == 0xFF27C08A);
    CHECK(violet.textTitle == 0xFFE9DCFF);

    // Pale accents carry DARK text on accent; saturated accents carry white.
    CHECK(themeTokens("nord").textOnAccent == 0xFF2E3440);
    CHECK(themeTokens("dracula").textOnAccent == 0xFF21222C);
    CHECK(themeTokens("catppuccin").textOnAccent == 0xFF1E1E2E);
    CHECK(themeTokens("tokyo-night").textOnAccent == 0xFF16161E);
    CHECK(themeTokens("one-dark").textOnAccent == 0xFF21252B);
    CHECK(themeTokens("midnight").textOnAccent == 0xFF0A0E1A);
    CHECK(themeTokens("solarized").textOnAccent == 0xFFFDF6E3);
    CHECK(themeTokens("violet").textOnAccent == 0xFFFFFFFF);
    CHECK(themeTokens("github-dark").textOnAccent == 0xFFFFFFFF);

    // Per-palette meter details.
    CHECK(themeTokens("oled-black").meterTrack == 0xFF000000);
    CHECK(themeTokens("graphite").meterMid == 0xFFD9A62E);
    CHECK(themeTokens("light").meterTrack == 0xFFDCE3EE);

    // "Meters follow semantics" palettes.
    const auto nord = themeTokens("nord");
    CHECK(nord.meterLow == nord.success);
    CHECK(nord.meterMid == nord.warning);
    CHECK(nord.meterHigh == nord.danger);
    CHECK(nord.meterTrack == nord.surfaceSunken);
}

TEST_CASE("Themes: unknown and legacy ids fall back to real palettes")
{
    CHECK(themeTokens("does-not-exist").accent == themeTokens("violet").accent);
    CHECK(themeTokens("pure-black").surfaceApp == themeTokens("oled-black").surfaceApp);
    CHECK(themeTokens("daylight").dark == false);
    CHECK(themeTokens("mono").accent == themeTokens("graphite").accent);
    CHECK(themeTokens("ocean").accent == themeTokens("blue").accent);
}

// --- verification 3: contrast spot-checks in every palette ---------------------

TEST_CASE("Themes: textPrimary on surfacePanel and textOnAccent on accent stay readable")
{
    for (const auto& info : themeCatalog())
    {
        const auto t = tokensFor(info.id);
        INFO("theme: " << info.id);
        CHECK(contrastRatio(t.textPrimary, t.surfacePanel) >= 4.5f);
        CHECK(contrastRatio(t.textOnAccent, t.accent) >= 3.0f);
    }
}

// --- Custom derivation ------------------------------------------------------------

TEST_CASE("Custom: anchors derive the full palette per the spec's rules")
{
    const auto t = deriveCustom(sampleAnchors());

    CHECK(t.surfaceApp == *parseHexColor("#101418"));
    CHECK(t.accent == *parseHexColor("#FF6B35"));

    // The surface ramp: panel/raised/nav/borders lighter than app, sunken darker.
    CHECK(relativeLuminance(t.surfacePanel) > relativeLuminance(t.surfaceApp));
    CHECK(relativeLuminance(t.surfaceRaised) > relativeLuminance(t.surfacePanel));
    CHECK(relativeLuminance(t.surfaceSunken) < relativeLuminance(t.surfaceApp));
    CHECK(relativeLuminance(t.surfaceNav) > relativeLuminance(t.surfaceApp));
    CHECK(relativeLuminance(t.borderStrong) > relativeLuminance(t.border));
    CHECK(t.meterTrack == t.surfaceSunken);

    // Accent family bracket the anchor.
    CHECK(relativeLuminance(t.accentBright) > relativeLuminance(t.accent));
    CHECK(relativeLuminance(t.accentDeep) < relativeLuminance(t.accent));
}

TEST_CASE("Custom: semantics drive the meters unless meter anchors are explicit")
{
    CustomAnchors a;
    a.success = parseHexColor("#11AA55");
    a.warning = parseHexColor("#CC9900");
    a.danger  = parseHexColor("#DD2222");
    auto t = deriveCustom(a);
    CHECK(t.meterLow == *a.success);
    CHECK(t.meterMid == *a.warning);
    CHECK(t.meterHigh == *a.danger);

    a.meterMid = parseHexColor("#FFAA00"); // explicit override wins
    t = deriveCustom(a);
    CHECK(t.meterMid == *a.meterMid);
    CHECK(t.meterLow == *a.success);
}

// --- verification 4 (data half): theme files + settings round-trips ----------------

TEST_CASE("Theme files: export -> import reproduces the identical palette")
{
    ThemeFile out;
    out.themeName = kCustomThemeId;
    out.custom    = sampleAnchors();

    const auto text = exportThemeFile(out);
    const auto in   = importThemeFile(text);
    REQUIRE(in.has_value());
    CHECK(in->themeName == out.themeName);

    const auto a = deriveCustom(out.custom);
    const auto b = deriveCustom(in->custom);
    CHECK(a.surfaceApp == b.surfaceApp);
    CHECK(a.accent == b.accent);
    CHECK(a.textOnAccent == b.textOnAccent);
    CHECK(a.meterTrack == b.meterTrack);
}

TEST_CASE("Theme files: the 64 KB cap and malformed input are rejected")
{
    CHECK_FALSE(importThemeFile(std::string(kThemeFileMaxBytes + 1, ' ')).has_value());
    CHECK_FALSE(importThemeFile("not json at all").has_value());
    CHECK_FALSE(importThemeFile("{}").has_value());          // themeName required
    CHECK_FALSE(importThemeFile("[1,2,3]").has_value());
    CHECK(importThemeFile("{\"themeName\":\"violet\"}").has_value());
}

TEST_CASE("Config: themeName + customTheme + reducedMotion persist")
{
    veyra::Config c;
    CHECK(c.theme == "violet"); // the default theme is purple

    c.theme        = "dracula";
    c.customTheme  = anchorsToJson(sampleAnchors());
    c.reduceMotion = true;

    const auto rt = veyra::Config::fromJson(c.toJson());
    REQUIRE(rt.has_value());
    CHECK(rt->theme == "dracula");
    CHECK(rt->reduceMotion == true);
    const auto restored = anchorsFromJson(rt->customTheme);
    REQUIRE(restored.accent.has_value());
    CHECK(*restored.accent == *parseHexColor("#FF6B35"));
    REQUIRE(restored.background.has_value());
    CHECK(*restored.background == *parseHexColor("#101418"));
}

// --- colour utilities ---------------------------------------------------------------

TEST_CASE("Hex colours parse and format symmetrically")
{
    CHECK(*parseHexColor("#8B5CF6") == 0xFF8B5CF6);
    CHECK(*parseHexColor("8b5cf6") == 0xFF8B5CF6);
    CHECK(*parseHexColor("#CC05080F") == 0xCC05080F);
    CHECK_FALSE(parseHexColor("#12345").has_value());
    CHECK_FALSE(parseHexColor("#GGGGGG").has_value());
    CHECK(formatHexColor(0xFF8B5CF6) == "#8B5CF6");
}

// --- motion tokens ----------------------------------------------------------------

TEST_CASE("Motion: fast/med/slow tokens, all zero under reduced motion")
{
    CHECK(motion::fastMs(false) == 120);
    CHECK(motion::medMs(false) == 180);
    CHECK(motion::slowMs(false) == 240);
    CHECK(motion::fastMs(true) == 0);
    CHECK(motion::medMs(true) == 0);
    CHECK(motion::slowMs(true) == 0);
}
