#pragma once

// Pill toggle switch (design §7.5): 36x20 track, 16px white thumb, accent ON
// track. A juce::Button so it carries toggle state and click handling.

#include "Theme/DesignTokens.h"
#include "VeyraGui.h"

namespace veyra::ui {

class ToggleSwitch : public juce::Button {
public:
    ToggleSwitch();
    void setPalette(const Palette& p) { palette_ = p; repaint(); }

    void paintButton(juce::Graphics&, bool shouldDrawHighlighted, bool shouldDrawDown) override;

private:
    Palette palette_ = paletteForTheme("midnight");
};

} // namespace veyra::ui
