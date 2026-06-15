#pragma once

// Segmented control (design §7.12): pill container, glass bg, active segment
// uses elevated glass + accent stroke. Used for Graphic/Parametric, mini-mode
// preset categories, etc.

#include "Theme/DesignTokens.h"
#include "VeyraGui.h"

#include <functional>

namespace veyra::ui {

class SegmentedControl : public juce::Component {
public:
    void setItems(juce::StringArray items) { items_ = std::move(items); repaint(); }
    void setSelectedIndex(int i, bool notify = true);
    int selectedIndex() const noexcept { return selected_; }
    void setPalette(const Palette& p) { palette_ = p; repaint(); }

    std::function<void(int)> onChange;

    void paint(juce::Graphics&) override;
    void mouseDown(const juce::MouseEvent&) override;

private:
    juce::Rectangle<float> segmentBounds(int index) const;

    juce::StringArray items_;
    int selected_ = 0;
    Palette palette_ = paletteForTheme("midnight");
};

} // namespace veyra::ui
