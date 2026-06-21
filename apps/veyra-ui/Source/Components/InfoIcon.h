#pragma once

// Small circled-"i" hint icon. Hovering shows a tooltip (via the app's
// TooltipWindow) describing the feature, what it does, and its shortcut if any.
// Header-only.

#include "Theme/DesignTokens.h"
#include "Theme/Fonts.h"
#include "VeyraGui.h"

namespace veyra::ui {

class InfoIcon : public juce::Component, public juce::SettableTooltipClient {
public:
    InfoIcon() { setInterceptsMouseClicks(true, false); }

    void setPalette(const Palette& p) { palette_ = p; repaint(); }

    void mouseEnter(const juce::MouseEvent&) override { hover_ = true; repaint(); }
    void mouseExit(const juce::MouseEvent&) override { hover_ = false; repaint(); }

    void paint(juce::Graphics& g) override
    {
        auto b = getLocalBounds().toFloat().reduced(1.0f);
        const auto c = hover_ ? palette_.accentPrimary : palette_.textTertiary;
        g.setColour(c);
        g.drawEllipse(b, 1.2f);
        g.setFont(fonts::body(juce::jmin(b.getHeight() * 0.72f, 11.0f), true));
        g.drawText("i", getLocalBounds(), juce::Justification::centred, false);
    }

private:
    Palette palette_ = paletteForTheme("midnight");
    bool    hover_ = false;
};

} // namespace veyra::ui
