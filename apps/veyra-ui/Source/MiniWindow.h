#pragma once

// Mini mode: a compact, always-on-top acrylic widget with the essentials —
// brand mark + connection LED, current preset, master mute + volume, slim peak
// meters, and expand/close. Shares the shell's config/client; the shell
// (RootComponent) owns the window and wires the callbacks.

#include "Components/IconButton.h"
#include "Components/ToggleSwitch.h"
#include "Graphics/Icons.h"
#include "Theme/DesignTokens.h"
#include "VeyraGui.h"

#include <array>
#include <functional>

namespace veyra::ui {

class MiniContent : public juce::Component, private juce::Timer {
public:
    MiniContent();
    ~MiniContent() override;

    void setPalette(const Palette&);
    void setMasterEnabled(bool on);
    void setMasterVolume(double gain);
    void setConnection(bool connected);
    void setPreset(juce::String name);

    std::function<void(bool)>   onMasterToggle;
    std::function<void(double)> onMasterVolume;
    std::function<void()>       onExpand;
    std::function<void()>       onClose;

    void paint(juce::Graphics&) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent&) override;
    void mouseDrag(const juce::MouseEvent&) override;

private:
    void timerCallback() override;

    Palette      palette_ = paletteForTheme("midnight");
    bool         connected_ = false;
    juce::String preset_ = "Custom";
    juce::Image  logoImage_;

    ToggleSwitch master_;
    juce::Slider volume_;
    IconButton   expand_{icons::fullscreen};
    IconButton   close_{icons::close, true};

    std::array<float, 2> peak_{0.4f, 0.3f};
    std::array<float, 2> peakTarget_{0.4f, 0.3f};
    juce::Random rng_;
    int frame_ = 0;

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
