#pragma once

// Fallback screen shown when a navIndex doesn't map to any built screen.
// All primary screens (Home, Presets, Apps, Devices, GamerMode, Settings) have
// dedicated implementations; this is only reached for unmapped indices.

#include "Components/GlassPanel.h"
#include "Theme/Fonts.h"
#include "VeyraGui.h"

namespace veyra::ui {

class PlaceholderScreen : public juce::Component {
public:
    PlaceholderScreen()
    {
        card_.setElevated(true);
        addAndMakeVisible(card_);
    }

    void setTitle(juce::String title) { card_.setTitle(std::move(title)); }
    void setPalette(const Palette& p) { card_.setPalette(p); }
    void attachBackdrop(GlassBackground* b) { card_.setBackdrop(b); }

    void resized() override { card_.setBounds(getLocalBounds()); }

private:
    class Card : public GlassPanel {
    public:
        void setTitle(juce::String t) { title_ = std::move(t); repaint(); }

    protected:
        void paintContent(juce::Graphics& g) override
        {
            auto b = getLocalBounds();
            g.setColour(palette_.textPrimary);
            g.setFont(fonts::display(24.0f));
            g.drawText(title_, b.removeFromTop(b.getHeight() / 2).withTrimmedBottom(8),
                       juce::Justification::centredBottom, false);
            g.setColour(palette_.textTertiary);
            g.setFont(fonts::body(14.0f));
            g.drawText("Coming soon", b.withTrimmedTop(8), juce::Justification::centredTop, false);
        }

    private:
        juce::String title_;
    };

    Card card_;
};

} // namespace veyra::ui
