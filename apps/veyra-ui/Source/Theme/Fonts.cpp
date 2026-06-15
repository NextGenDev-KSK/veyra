#include "Theme/Fonts.h"

namespace veyra::ui::fonts {

namespace {
juce::Typeface::Ptr g_display, g_body, g_mono;

juce::Font make(juce::Typeface::Ptr tf, float height)
{
    return juce::Font(juce::FontOptions().withTypeface(tf).withHeight(height));
}
} // namespace

void initialise()
{
    if (g_body != nullptr)
        return;

    g_display = juce::Typeface::createSystemTypefaceFor(
        BinaryData::Orbitron_ttf, BinaryData::Orbitron_ttfSize);
    g_body = juce::Typeface::createSystemTypefaceFor(
        BinaryData::Inter_ttf, BinaryData::Inter_ttfSize);
    g_mono = juce::Typeface::createSystemTypefaceFor(
        BinaryData::JetBrainsMono_ttf, BinaryData::JetBrainsMono_ttfSize);
}

juce::Font display(float height)
{
    // Orbitron with the spec's +0.06em tracking (approximated via kerning).
    return make(g_display, height).withExtraKerningFactor(0.06f);
}

juce::Font body(float height, bool semibold)
{
    auto f = make(g_body, height);
    return semibold ? f.boldened() : f;
}

juce::Font mono(float height, bool medium)
{
    auto f = make(g_mono, height);
    return medium ? f.boldened() : f;
}

} // namespace veyra::ui::fonts
