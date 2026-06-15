#pragma once

// Semantic colour tokens (from reference/design/design-tokens.json), expressed
// as a Palette. Midnight is the canonical base; the other built-in themes are
// overrides on top of it. Hard-coded (rather than parsed at runtime) so the
// theme path has no I/O or JSON dependency; runtime/custom themes can layer on
// later.

#include "VeyraGui.h"

#include <vector>

namespace veyra::ui {

struct Palette {
    bool dark = true;

    juce::Colour bgApp, bgCanvas, bgGlass, bgGlassHover, bgGlassActive,
                 bgGlassElevated, bgModalScrim, bgInput;

    juce::Colour strokeDefault, strokeHover, strokeActive, strokeFocus, strokeLightLeak;

    juce::Colour textPrimary, textSecondary, textTertiary, textDisabled, textOnAccent;

    juce::Colour accentPrimary, accentPrimaryHover, accentPrimaryActive,
                 accentGlow, accentGlowStrong, accentSecondary, accentSecondaryDim;

    juce::Colour success, successBg, warning, warningBg, danger, dangerBg, info, infoBg;
};

struct ThemeInfo {
    juce::String id;
    juce::String name;
};

// The 11 built-in themes, in display order.
const std::vector<ThemeInfo>& builtInThemes();

// Palette for a theme id (falls back to Midnight for unknown ids).
Palette paletteForTheme(const juce::String& id);

} // namespace veyra::ui
