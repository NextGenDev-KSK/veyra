#pragma once

// Mini mode: a compact, always-on-top widget matching the reference — brand mark,
// preset name + category, previous/next preset, a live visualizer over the volume
// slider with a % readout, Spatial + Game shortcuts, a menu, and close. A
// Visualizer-Only mode hides every control and fills the width with the spectrum.
// Shares the shell's config/client; the shell (RootComponent) owns the window.

#include "Components/IconButton.h"
#include "Graphics/Icons.h"
#include "Theme/DesignTokens.h"
#include "VeyraGui.h"

#include <functional>
#include <vector>

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
    void setCategory(juce::String category);
    void setVisualizerBars(const float* bars, int n); // live spectrum feed

    std::function<void(bool)>   onMasterToggle;   // brand mark toggles master
    std::function<void(double)> onMasterVolume;
    std::function<void()>       onExpand;
    std::function<void()>       onClose;
    std::function<void()>       onPrevPreset;
    std::function<void()>       onNextPreset;
    std::function<void()>       onSpatial;
    std::function<void()>       onGame;

    void paint(juce::Graphics&) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent&) override;
    void mouseDrag(const juce::MouseEvent&) override;

private:
    void timerCallback() override;
    void showMenu();
    void setVisualizerOnly(bool on);
    void drawVisualizer(juce::Graphics&, juce::Rectangle<float> area);

    Palette      palette_ = paletteForTheme("midnight");
    bool         connected_ = false;
    bool         masterEnabled_ = true;
    bool         visualizerOnly_ = false;
    juce::String preset_ = "Custom";
    juce::String category_;
    juce::Image  logoImage_;

    juce::Slider volume_;
    IconButton   prev_{icons::chevronLeft};
    IconButton   next_{icons::chevronRight};
    IconButton   spatial_{icons::spatial};
    IconButton   game_{icons::gamer};
    IconButton   menu_{icons::dots};
    IconButton   close_{icons::close, true};

    std::vector<float> bars_;       // smoothed spectrum
    juce::Point<int>   dragWin_, dragMouse_;
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
