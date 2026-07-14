#include "Theme/DesignTokens.h"

namespace veyra::ui {

namespace {

juce::Colour col(veyra::theme::Argb argb) { return juce::Colour((juce::uint32) argb); }

} // namespace

const std::vector<ThemeInfo>& builtInThemes()
{
    static const std::vector<ThemeInfo> themes = []
    {
        std::vector<ThemeInfo> out;
        for (const auto& t : veyra::theme::themeCatalog())
            out.push_back({juce::String(t.id), juce::String(t.name)});
        return out;
    }();
    return themes;
}

Palette paletteFromTokens(const veyra::theme::ThemeTokens& t)
{
    using veyra::theme::withAlpha;

    Palette p;
    p.dark = t.dark;

    // Surfaces. The frosted-glass panels keep their translucency by carrying the
    // panel/raised colours with an alpha; depth otherwise comes from the ramp.
    p.bgApp           = col(t.surfaceApp);
    p.bgCanvas        = col(t.surfacePanel);
    p.bgGlass         = col(withAlpha(t.surfacePanel,  t.dark ? 0.42f : 0.55f));
    p.bgGlassHover    = col(withAlpha(t.surfaceRaised, t.dark ? 0.55f : 0.70f));
    p.bgGlassActive   = col(withAlpha(t.surfaceRaised, t.dark ? 0.62f : 0.78f));
    p.bgGlassElevated = col(withAlpha(t.surfaceRaised, t.dark ? 0.60f : 0.75f));
    p.bgModalScrim    = col(veyra::theme::ThemeTokens::scrim());
    p.bgInput         = col(t.surfaceSunken);
    p.bgNav           = col(t.surfaceNav);

    p.strokeDefault   = col(t.border);
    p.strokeHover     = col(t.borderStrong);
    p.strokeActive    = col(t.borderGlow());
    p.strokeFocus     = col(withAlpha(t.accent, 0.80f));
    p.strokeLightLeak = juce::Colours::white.withAlpha(0.10f);

    p.textPrimary   = col(t.textPrimary);
    p.textSecondary = col(t.textSecondary);
    p.textTertiary  = col(t.textMuted);
    p.textDisabled  = col(t.textMuted).withAlpha(0.55f);
    p.textOnAccent  = col(t.textOnAccent);
    p.textTitle     = col(t.textTitle);

    p.accentPrimary       = col(t.accent);
    p.accentPrimaryHover  = col(t.accentBright);
    p.accentPrimaryActive = col(t.accentDeep);
    p.accentGlow          = col(withAlpha(t.accent, 0.40f));
    p.accentGlowStrong    = col(withAlpha(t.accent, 0.65f));
    p.accentWash          = col(t.accentWash());
    p.accentSecondary     = col(t.accentBright);
    p.accentSecondaryDim  = col(t.accentDeep);

    p.success = col(t.success); p.successBg = col(withAlpha(t.success, 0.15f));
    p.warning = col(t.warning); p.warningBg = col(withAlpha(t.warning, 0.15f));
    p.danger  = col(t.danger);  p.dangerBg  = col(withAlpha(t.danger, 0.15f));
    p.info    = col(t.accentBright); p.infoBg = col(withAlpha(t.accentBright, 0.15f));

    p.meterLow   = col(t.meterLow);
    p.meterMid   = col(t.meterMid);
    p.meterHigh  = col(t.meterHigh);
    p.meterTrack = col(t.meterTrack);

    return p;
}

Palette paletteForTheme(const juce::String& id)
{
    return paletteFromTokens(veyra::theme::themeTokens(id.toStdString()));
}

} // namespace veyra::ui
