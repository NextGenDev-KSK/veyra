#pragma once

// Left navigation rail: 7 items (Home..Settings) with icons + active accent
// rail, a Mini Mode button, and the version label.

#include "Theme/DesignTokens.h"
#include "VeyraGui.h"

#include <functional>
#include <vector>

namespace veyra::ui {

class Sidebar : public juce::Component {
public:
    using IconFn = void (*)(juce::Graphics&, juce::Rectangle<float>, juce::Colour);

    Sidebar();

    void setPalette(const Palette& p) { palette_ = p; repaint(); }
    std::function<void(int)> onNavigate;

    void paint(juce::Graphics&) override;
    void mouseDown(const juce::MouseEvent&) override;
    void mouseMove(const juce::MouseEvent&) override;
    void mouseExit(const juce::MouseEvent&) override;

private:
    struct Item { juce::String label; IconFn icon; };
    juce::Rectangle<int> itemBounds(int i) const;
    juce::Rectangle<int> miniBounds() const;

    std::vector<Item> items_;
    int active_ = 0;
    int hover_ = -1;
    Palette palette_ = paletteForTheme("midnight");
};

} // namespace veyra::ui
