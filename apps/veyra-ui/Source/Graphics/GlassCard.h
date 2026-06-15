#pragma once

// The structural glass surface (design §2.1). A container that paints the glass
// recipe — translucent tint, 1px inner stroke, top light-leak, soft shadow —
// with an optional Nasalization/Orbitron title row. Children are positioned by
// the owner inside contentArea().

#include "Theme/DesignTokens.h"
#include "VeyraGui.h"

namespace veyra::ui {

// Reusable glass painter so non-card surfaces (top bar, mini-mode) can match.
void paintGlassSurface(juce::Graphics&, juce::Rectangle<float> bounds,
                       const Palette&, float radius, bool elevated);

class GlassCard : public juce::Component {
public:
    void setPalette(const Palette& p) { palette_ = p; repaint(); }
    void setTitle(juce::String t) { title_ = std::move(t); repaint(); }
    void setElevated(bool e) { elevated_ = e; repaint(); }

    // Inner area available for content (below the title row, inside padding).
    juce::Rectangle<int> contentArea() const;

    void paint(juce::Graphics&) override;

private:
    Palette palette_ = paletteForTheme("midnight");
    juce::String title_;
    bool elevated_ = false;
    static constexpr int kPadding = 24;
    static constexpr int kRadius = 16;
};

} // namespace veyra::ui
