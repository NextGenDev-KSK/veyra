#pragma once

// A 32x32 icon-only button (design §7.4): hover glass fill, colour shifts by
// state. The icon is drawn by a supplied function from Graphics/Icons.h.

#include "Theme/DesignTokens.h"
#include "VeyraGui.h"

#include <functional>

namespace veyra::ui {

class IconButton : public juce::Button {
public:
    using Drawer = std::function<void(juce::Graphics&, juce::Rectangle<float>, juce::Colour)>;

    explicit IconButton(Drawer drawer, bool isClose = false)
        : juce::Button({}), draw_(std::move(drawer)), isClose_(isClose) {}

    void setPalette(const Palette& p) { palette_ = p; repaint(); }

    void paintButton(juce::Graphics& g, bool over, bool down) override
    {
        auto b = getLocalBounds().toFloat();
        if (over)
        {
            g.setColour(isClose_ ? palette_.danger.withAlpha(0.18f) : palette_.bgGlassHover);
            g.fillRoundedRectangle(b, 8.0f);
        }
        juce::Colour col = down ? palette_.accentPrimary
                           : over ? (isClose_ ? juce::Colours::white : palette_.textPrimary)
                                  : palette_.textSecondary;
        if (draw_)
            draw_(g, b, col);
    }

private:
    Drawer draw_;
    bool isClose_ = false;
    Palette palette_ = paletteForTheme("midnight");
};

} // namespace veyra::ui
