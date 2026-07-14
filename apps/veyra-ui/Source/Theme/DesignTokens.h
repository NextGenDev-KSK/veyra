#pragma once

// The view-side palette. Themes are DATA: every palette is produced from a
// veyra::theme::ThemeTokens (see modules/veyra-common/include/veyra/ThemeTokens.h,
// where the canonical curated values live). This header only adapts those
// tokens to the juce::Colour fields the components consume — no palette values
// are defined here, and no component may hardcode a colour.

#include "VeyraGui.h"

#include "veyra/ThemeTokens.h"

#include <vector>

namespace veyra::ui {

struct Palette {
    bool dark = true;

    juce::Colour bgApp, bgCanvas, bgGlass, bgGlassHover, bgGlassActive,
                 bgGlassElevated, bgModalScrim, bgInput, bgNav;

    juce::Colour strokeDefault, strokeHover, strokeActive, strokeFocus, strokeLightLeak;

    juce::Colour textPrimary, textSecondary, textTertiary, textDisabled, textOnAccent,
                 textTitle;

    juce::Colour accentPrimary, accentPrimaryHover, accentPrimaryActive,
                 accentGlow, accentGlowStrong, accentWash,
                 accentSecondary, accentSecondaryDim;

    juce::Colour success, successBg, warning, warningBg, danger, dangerBg, info, infoBg;

    // Audio meter scale (never reused for chrome).
    juce::Colour meterLow, meterMid, meterHigh, meterTrack;
};

struct ThemeInfo {
    juce::String id;
    juce::String name;
};

// All themes in gallery order (Violet default first, Custom last).
const std::vector<ThemeInfo>& builtInThemes();

// Adapt an engine palette to the view-side Palette.
Palette paletteFromTokens(const veyra::theme::ThemeTokens& tokens);

// Palette for a curated theme id (legacy/unknown ids fall back to Violet).
Palette paletteForTheme(const juce::String& id);

} // namespace veyra::ui
