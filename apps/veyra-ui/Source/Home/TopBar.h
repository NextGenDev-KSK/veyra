#pragma once

// Custom glass top utility bar (replaces the native title bar): logo + wordmark,
// master toggle, master volume, preset chip, A/B, search/bell/gear, and window
// controls. Also drags the borderless window.

#include "Components/IconButton.h"
#include "Components/ToggleSwitch.h"
#include "Graphics/Icons.h"
#include "Theme/DesignTokens.h"
#include "VeyraGui.h"

namespace veyra::ui {

class TopBar : public juce::Component {
public:
    TopBar();

    void setPalette(const Palette&);
    void paint(juce::Graphics&) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent&) override;
    void mouseDrag(const juce::MouseEvent&) override;
    void mouseDoubleClick(const juce::MouseEvent&) override;

private:
    void toggleMaximise();

    Palette palette_ = paletteForTheme("midnight");

    ToggleSwitch master_;
    juce::Slider volume_;
    juce::TextButton ab_{"A/B"};
    IconButton search_{icons::search};
    IconButton bell_{icons::bell};
    IconButton gear_{icons::gear};
    IconButton min_{icons::minimise};
    IconButton max_{icons::maximise};
    IconButton close_{icons::close, true};

    juce::Point<int> dragWin_, dragMouse_;
};

} // namespace veyra::ui
