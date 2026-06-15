#pragma once

// paintGlass: renders the glass recipe (frosted backdrop slice + tint + 1px
// stroke + top light-leak + soft shadow) for a component, sampling the blurred
// background so the bright ambient blobs show through tinted — the premium
// frosted look that a flat translucent fill can't achieve on a dark canvas.

#include "Graphics/GlassBackground.h"
#include "Theme/DesignTokens.h"
#include "VeyraGui.h"

namespace veyra::ui {

inline void paintGlass(juce::Graphics& g, juce::Component& card, GlassBackground* backdrop,
                       const Palette& p, float radius, bool elevated)
{
    const auto b = card.getLocalBounds().toFloat();

    juce::DropShadow(juce::Colours::black.withAlpha(0.35f), 24, {0, 8})
        .drawForRectangle(g, card.getLocalBounds());

    if (backdrop != nullptr && backdrop->blurred().isValid())
    {
        const auto& bl = backdrop->blurred();
        const auto origin = backdrop->getLocalPoint(&card, juce::Point<int>(0, 0));
        const float sx = (float) backdrop->getWidth() / (float) bl.getWidth();
        const float sy = (float) backdrop->getHeight() / (float) bl.getHeight();

        juce::Path rr;
        rr.addRoundedRectangle(b, radius);

        juce::Graphics::ScopedSaveState save(g);
        g.reduceClipRegion(rr);

        const auto t = juce::AffineTransform::scale(sx, sy)
                           .translated((float) -origin.x, (float) -origin.y);
        g.drawImageTransformed(bl, t, false);

        g.setColour(elevated ? p.bgGlassElevated : p.bgGlass);
        g.fillRect(b); // tint over the frosted slice (clipped to the rounded rect)
    }
    else
    {
        g.setColour(elevated ? p.bgGlassElevated : p.bgGlass);
        g.fillRoundedRectangle(b, radius);
    }

    g.setColour(p.strokeLightLeak);
    g.drawLine(b.getX() + radius, b.getY() + 1.0f, b.getRight() - radius, b.getY() + 1.0f, 1.0f);
    g.setColour(p.strokeDefault);
    g.drawRoundedRectangle(b.reduced(0.5f), radius, 1.0f);
}

} // namespace veyra::ui
