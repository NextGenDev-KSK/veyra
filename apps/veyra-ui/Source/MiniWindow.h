#pragma once

// Mini mode — Phase 4c. A compact, always-on-top, borderless bar with the
// essentials (master toggle + volume) plus expand/close. It shares the shell's
// working Config and service client: changes here push the same parameters and
// the bar reflects config/connection updates. The shell (RootComponent) owns
// the window and wires the callbacks.

#include "Components/IconButton.h"
#include "Components/ToggleSwitch.h"
#include "Graphics/Icons.h"
#include "Theme/DesignTokens.h"
#include "VeyraGui.h"

#include <functional>

namespace veyra::ui {

class MiniContent : public juce::Component {
public:
    MiniContent();

    void setPalette(const Palette&);
    void setMasterEnabled(bool on);
    void setMasterVolume(double gain);
    void setConnection(bool connected);

    std::function<void(bool)>   onMasterToggle;
    std::function<void(double)> onMasterVolume;
    std::function<void()>       onExpand;
    std::function<void()>       onClose;

    void paint(juce::Graphics&) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent&) override;
    void mouseDrag(const juce::MouseEvent&) override;

private:
    Palette palette_ = paletteForTheme("midnight");
    bool    connected_ = false;

    ToggleSwitch master_;
    juce::Slider volume_;
    IconButton   expand_{icons::fullscreen};
    IconButton   close_{icons::close, true};

    juce::Point<int> dragWin_, dragMouse_;
};

class MiniWindow : public juce::DocumentWindow {
public:
    MiniWindow();

    MiniContent& content() { return *content_; }

private:
    MiniContent* content_ = nullptr; // owned by the DocumentWindow

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MiniWindow)
};

} // namespace veyra::ui
