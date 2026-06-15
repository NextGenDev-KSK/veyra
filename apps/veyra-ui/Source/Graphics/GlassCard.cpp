#include "Graphics/GlassCard.h"

#include "Theme/Fonts.h"

namespace veyra::ui {

void paintGlassSurface(juce::Graphics& g, juce::Rectangle<float> b, const Palette& p,
                       float radius, bool elevated)
{
    // Soft outer shadow (definition without heavy drop-shadows).
    juce::DropShadow(juce::Colours::black.withAlpha(0.35f), 24, {0, 8})
        .drawForRectangle(g, b.toNearestInt());

    g.setColour(elevated ? p.bgGlassElevated : p.bgGlass);
    g.fillRoundedRectangle(b, radius);

    // Top "light-leak" edge: a 1px brighter line just inside the top.
    g.setColour(p.strokeLightLeak);
    g.drawLine(b.getX() + radius, b.getY() + 1.0f,
               b.getRight() - radius, b.getY() + 1.0f, 1.0f);

    // 1px inner border.
    g.setColour(p.strokeDefault);
    g.drawRoundedRectangle(b.reduced(0.5f), radius, 1.0f);
}

juce::Rectangle<int> GlassCard::contentArea() const
{
    auto r = getLocalBounds().reduced(kPadding);
    if (title_.isNotEmpty())
        r.removeFromTop(28 + 16); // title row + 16px gap
    return r;
}

void GlassCard::paint(juce::Graphics& g)
{
    paintGlassSurface(g, getLocalBounds().toFloat(), palette_, (float) kRadius, elevated_);

    if (title_.isNotEmpty())
    {
        auto titleRow = getLocalBounds().reduced(kPadding).removeFromTop(28);
        g.setColour(palette_.textPrimary);
        g.setFont(fonts::display(20.0f));
        g.drawText(title_.toUpperCase(), titleRow, juce::Justification::centredLeft, true);
    }
}

} // namespace veyra::ui
