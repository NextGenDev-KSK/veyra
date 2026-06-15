#include "Theme/DesignTokens.h"

namespace veyra::ui {

namespace {

juce::Colour hex(juce::uint32 rgb) { return juce::Colour(0xff000000 | rgb); }
juce::Colour rgba(juce::uint8 r, juce::uint8 g, juce::uint8 b, float a)
{
    return juce::Colour(r, g, b).withAlpha(a);
}

Palette midnight()
{
    Palette p;
    p.dark = true;

    p.bgApp          = hex(0x0A0B10);
    p.bgCanvas       = hex(0x0F1117);
    p.bgGlass        = rgba(20, 22, 32, 0.55f);
    p.bgGlassHover   = rgba(28, 30, 42, 0.65f);
    p.bgGlassActive  = rgba(36, 38, 52, 0.75f);
    p.bgGlassElevated= rgba(32, 34, 48, 0.82f);
    p.bgModalScrim   = rgba(0, 0, 0, 0.55f);
    p.bgInput        = hex(0x07080C);

    p.strokeDefault  = rgba(255, 255, 255, 0.08f);
    p.strokeHover    = rgba(255, 255, 255, 0.14f);
    p.strokeActive   = rgba(124, 92, 255, 0.45f);
    p.strokeFocus    = rgba(124, 92, 255, 0.80f);
    p.strokeLightLeak= rgba(255, 255, 255, 0.06f);

    p.textPrimary    = hex(0xF2F3F8);
    p.textSecondary  = hex(0xA8ABBD);
    p.textTertiary   = hex(0x6B6F84);
    p.textDisabled   = hex(0x4A4D5E);
    p.textOnAccent   = hex(0xFFFFFF);

    p.accentPrimary       = hex(0x7C5CFF);
    p.accentPrimaryHover  = hex(0x8E72FF);
    p.accentPrimaryActive = hex(0x6A4AE8);
    p.accentGlow          = rgba(124, 92, 255, 0.40f);
    p.accentGlowStrong    = rgba(124, 92, 255, 0.65f);
    p.accentSecondary     = hex(0xA8C64E);
    p.accentSecondaryDim  = hex(0x86A03E);

    p.success   = hex(0x4ADE80); p.successBg = rgba(74, 222, 128, 0.15f);
    p.warning   = hex(0xFBBF24); p.warningBg = rgba(251, 191, 36, 0.15f);
    p.danger    = hex(0xF87171); p.dangerBg  = rgba(248, 113, 113, 0.15f);
    p.info      = hex(0x60A5FA); p.infoBg    = rgba(96, 165, 250, 0.15f);
    return p;
}

// Apply an accent across the primary/hover/active/glow set.
void setAccent(Palette& p, juce::Colour accent)
{
    p.accentPrimary       = accent;
    p.accentPrimaryHover  = accent.brighter(0.15f);
    p.accentPrimaryActive = accent.darker(0.15f);
    p.accentGlow          = accent.withAlpha(0.40f);
    p.accentGlowStrong    = accent.withAlpha(0.65f);
    p.strokeActive        = accent.withAlpha(0.45f);
    p.strokeFocus         = accent.withAlpha(0.80f);
}

} // namespace

const std::vector<ThemeInfo>& builtInThemes()
{
    static const std::vector<ThemeInfo> themes = {
        {"midnight", "Midnight"},
        {"pure-black", "Pure Black"},
        {"daylight", "Daylight"},
        {"synthwave", "Synthwave"},
        {"cyberpunk", "Cyberpunk"},
        {"forest", "Forest"},
        {"sunset", "Sunset"},
        {"ocean", "Ocean"},
        {"mono", "Mono"},
        {"glassmorphism-max", "Glassmorphism Max"},
        {"custom", "Custom"},
    };
    return themes;
}

Palette paletteForTheme(const juce::String& id)
{
    Palette p = midnight();

    if (id == "pure-black")
    {
        p.bgApp = hex(0x000000);
        p.bgCanvas = hex(0x050507);
        p.bgGlass = rgba(10, 10, 14, 0.65f);
    }
    else if (id == "daylight")
    {
        p.dark = false;
        p.bgApp = hex(0xF4F5F8);
        p.bgCanvas = hex(0xFFFFFF);
        p.bgGlass = rgba(255, 255, 255, 0.55f);
        p.bgGlassHover = rgba(245, 246, 250, 0.70f);
        p.bgInput = hex(0xFAFAFC);
        p.strokeDefault = rgba(0, 0, 0, 0.08f);
        p.strokeHover = rgba(0, 0, 0, 0.14f);
        p.textPrimary = hex(0x0F1117);
        p.textSecondary = hex(0x5B5F73);
        p.textTertiary = hex(0x8A8E9F);
        setAccent(p, hex(0x5B3FE4));
    }
    else if (id == "synthwave")
    {
        p.bgApp = hex(0x1A0B2E);
        p.bgCanvas = hex(0x220F3D);
        p.bgGlass = rgba(40, 18, 70, 0.55f);
        setAccent(p, hex(0xFF3CAC));
        p.accentSecondary = hex(0x784BA0);
    }
    else if (id == "cyberpunk")
    {
        p.bgApp = hex(0x000000);
        p.bgCanvas = hex(0x080808);
        p.bgGlass = rgba(15, 15, 15, 0.65f);
        setAccent(p, hex(0xF0FF26));
        p.accentSecondary = hex(0x00E5FF);
    }
    else if (id == "forest")
    {
        p.bgApp = hex(0x0B1A0F);
        p.bgCanvas = hex(0x102216);
        p.bgGlass = rgba(20, 35, 25, 0.55f);
        setAccent(p, hex(0x86E08A));
    }
    else if (id == "sunset")
    {
        p.bgApp = hex(0x1F0E08);
        p.bgCanvas = hex(0x2B140C);
        p.bgGlass = rgba(50, 22, 14, 0.55f);
        setAccent(p, hex(0xFF6B4A));
        p.accentSecondary = hex(0xFFA94D);
    }
    else if (id == "ocean")
    {
        p.bgApp = hex(0x06181D);
        p.bgCanvas = hex(0x0A2229);
        p.bgGlass = rgba(15, 40, 50, 0.55f);
        setAccent(p, hex(0x3FD7E4));
    }
    else if (id == "mono")
    {
        p.bgApp = hex(0x0A0A0A);
        p.bgCanvas = hex(0x121212);
        p.bgGlass = rgba(28, 28, 28, 0.65f);
        setAccent(p, hex(0xFFFFFF));
    }
    else if (id == "glassmorphism-max")
    {
        p.bgGlass = rgba(20, 22, 32, 0.40f);
    }
    // "midnight", "custom", and unknown ids use the Midnight base.
    return p;
}

} // namespace veyra::ui
