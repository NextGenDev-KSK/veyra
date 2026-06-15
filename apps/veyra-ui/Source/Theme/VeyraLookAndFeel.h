#pragma once

// Applies the Veyra design tokens to JUCE's stock controls (text buttons,
// linear sliders, combo boxes) and exposes the active palette + fonts so bespoke
// components (knob, toggle, segmented control, glass card) can paint to match.

#include "Theme/DesignTokens.h"
#include "Theme/Fonts.h"
#include "VeyraGui.h"

namespace veyra::ui {

class VeyraLookAndFeel : public juce::LookAndFeel_V4 {
public:
    VeyraLookAndFeel();

    void setPalette(const Palette& p);
    const Palette& palette() const noexcept { return palette_; }

    // ---- Buttons ----
    juce::Font getTextButtonFont(juce::TextButton&, int buttonHeight) override;
    void drawButtonBackground(juce::Graphics&, juce::Button&,
                              const juce::Colour& backgroundColour,
                              bool shouldDrawButtonAsHighlighted,
                              bool shouldDrawButtonAsDown) override;
    void drawButtonText(juce::Graphics&, juce::TextButton&,
                        bool shouldDrawButtonAsHighlighted,
                        bool shouldDrawButtonAsDown) override;

    // ---- Sliders ----
    void drawLinearSlider(juce::Graphics&, int x, int y, int width, int height,
                          float sliderPos, float minSliderPos, float maxSliderPos,
                          juce::Slider::SliderStyle, juce::Slider&) override;

    // ---- Combo boxes ----
    void drawComboBox(juce::Graphics&, int width, int height, bool isButtonDown,
                      int buttonX, int buttonY, int buttonW, int buttonH,
                      juce::ComboBox&) override;
    juce::Font getComboBoxFont(juce::ComboBox&) override;

    // ---- Popup menus (used by dropdowns + tray later) ----
    void drawPopupMenuBackground(juce::Graphics&, int width, int height) override;

private:
    Palette palette_ = paletteForTheme("midnight");
};

} // namespace veyra::ui
