#pragma once

// Custom glass top utility bar (replaces the native title bar): logo + wordmark,
// master toggle, master volume, preset chip, A/B, search/bell/gear, and window
// controls. Also drags the borderless window.

#include "Components/IconButton.h"
#include "Components/ToggleSwitch.h"
#include "Graphics/Icons.h"
#include "Theme/DesignTokens.h"
#include "VeyraGui.h"

#include <functional>

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

    // Master controls — emitted on user interaction.
    std::function<void(bool)>   onMasterToggle;  // enabled
    std::function<void(double)> onMasterVolume;  // linear gain (0..slider max)

    // Refresh control state from config without firing the callbacks above.
    void setMasterEnabled(bool on);
    void setMasterVolume(double gain);

    // Connection status dot beside the wordmark.
    void setConnection(bool connected, juce::String version);

private:
    void toggleMaximise();

    Palette palette_ = paletteForTheme("midnight");

    bool         connected_ = false;
    juce::String version_;

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
