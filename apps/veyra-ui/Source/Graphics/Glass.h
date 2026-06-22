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

    // Shadow follows the rounded shape (drawForRectangle left a square corner
    // fringe that ignored the radius).
    juce::Path shape;
    shape.addRoundedRectangle(b, radius);
    juce::DropShadow(juce::Colours::black.withAlpha(0.35f), 24, {0, 8}).drawForPath(g, shape);

    if (backdrop != nullptr && backdrop->blurred().isValid())
    {
        const auto& bl = backdrop->blurred();
        const auto origin = backdrop->getLocalPoint(&card, juce::Point<int>(0, 0));
        const float sx = (float) backdrop->getWidth() / (float) bl.getWidth();
        const float sy = (float) backdrop->getHeight() / (float) bl.getHeight();
        const auto t = juce::AffineTransform::scale(sx, sy)
                           .translated((float) -origin.x, (float) -origin.y);

        // Fill the rounded rect *with* the blurred backdrop as the fill type, then
        // the tint on top — both via fillRoundedRectangle, which is anti-aliased
        // (the old reduceClipRegion(path) clip was not, hence the jagged/leaky
        // corners). The shape's corners are now mathematically clean.
        g.setFillType(juce::FillType(bl, t));
        g.fillRoundedRectangle(b, radius);
        g.setColour(elevated ? p.bgGlassElevated : p.bgGlass); // resets fill to solid
        g.fillRoundedRectangle(b, radius);
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
