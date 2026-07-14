#pragma once

// The theming engine's data layer: a theme is data, not code. One theme is one
// ThemeTokens palette; the UI reads every colour through the active palette and
// switching themes swaps the object (the view layer repaints live).
//
// Colours are ARGB (0xAARRGGBB) so this layer stays free of any UI framework
// and the whole engine is unit-testable. The curated palette values are
// canonical (community palettes included) — do not "improve" them here.

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace veyra::theme {

using Argb = std::uint32_t;

constexpr Argb kUnset = 0x00000000; // sentinel: token not defined (tests reject)

// --- colour math (shared by the Custom derivation and the UI layer) ----------
Argb  withAlpha(Argb c, float alpha);           // replace the alpha channel
Argb  scaleLightness(Argb c, float factor);     // HSL lightness * factor, clamped
float relativeLuminance(Argb c);                // WCAG 2.x, 0..1
float contrastRatio(Argb a, Argb b);            // WCAG 2.x, 1..21

// "#RRGGBB" / "#AARRGGBB" (case-insensitive). Returns nullopt on anything else.
std::optional<Argb> parseHexColor(const std::string& text);
std::string         formatHexColor(Argb c); // "#RRGGBB" (alpha dropped)

// --- the token set ------------------------------------------------------------
struct ThemeTokens {
    bool dark = true;

    // Surfaces: darkest -> lightest ramp; depth comes from the ramp, not shadows.
    Argb surfaceApp = kUnset, surfacePanel = kUnset, surfaceRaised = kUnset,
         surfaceSunken = kUnset, surfaceNav = kUnset;

    Argb border = kUnset, borderStrong = kUnset;

    Argb accent = kUnset, accentBright = kUnset, accentDeep = kUnset;

    Argb success = kUnset, warning = kUnset, danger = kUnset;

    // Audio meter scale — never reused for chrome.
    Argb meterLow = kUnset, meterMid = kUnset, meterHigh = kUnset, meterTrack = kUnset;

    Argb textPrimary = kUnset, textSecondary = kUnset, textMuted = kUnset,
         textOnAccent = kUnset, textTitle = kUnset;

    // Derived tokens.
    Argb borderGlow() const { return withAlpha(accent, 0.35f); }
    Argb accentWash() const { return withAlpha(accent, dark ? 0.14f : 0.10f); }

    // Overlay scrim: always dark, theme-independent. rgba(0.02, 0.03, 0.06, 0.80).
    static Argb scrim() { return 0xCC05080F; }
};

// --- curated themes -------------------------------------------------------------
struct ThemeInfo {
    std::string id;
    std::string name;
};

// All themes in gallery order: Violet (default) first, Custom last.
const std::vector<ThemeInfo>& themeCatalog();

constexpr const char* kDefaultThemeId = "violet";
constexpr const char* kCustomThemeId  = "custom";

// Palette for a curated theme id. Unknown ids (including legacy pre-port ids)
// fall back to the default (Violet).
ThemeTokens themeTokens(const std::string& id);

// --- the Custom theme -----------------------------------------------------------
// Users set a handful of anchors; the engine derives the full palette on top of
// the default (Violet) base.
struct CustomAnchors {
    std::optional<Argb> accent, background;
    std::optional<Argb> success, warning, danger;
    std::optional<Argb> meterLow, meterMid, meterHigh;
};

ThemeTokens deriveCustom(const CustomAnchors& anchors);

// Anchors <-> JSON object text (the `custom` block of a theme file, also stored
// verbatim in app settings).
std::string  anchorsToJson(const CustomAnchors& anchors);
CustomAnchors anchorsFromJson(const std::string& jsonText); // missing/invalid keys ignored

// --- theme files ------------------------------------------------------------------
// { "themeName": "...", "custom": { ... } } — `custom` present only when meaningful.
struct ThemeFile {
    std::string   themeName;
    CustomAnchors custom;
};

constexpr std::size_t kThemeFileMaxBytes = 64 * 1024;

std::string              exportThemeFile(const ThemeFile& file);
std::optional<ThemeFile> importThemeFile(const std::string& text); // nullopt: too big/invalid

// --- motion system -----------------------------------------------------------------
// Three duration tokens; every animation uses these. All return 0 when reduced
// motion is on (audio meters are never animated regardless).
namespace motion {
inline int fastMs(bool reducedMotion) { return reducedMotion ? 0 : 120; } // state feedback
inline int medMs(bool reducedMotion)  { return reducedMotion ? 0 : 180; } // movement
inline int slowMs(bool reducedMotion) { return reducedMotion ? 0 : 240; } // entrances
} // namespace motion

} // namespace veyra::theme
