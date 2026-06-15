#pragma once

// Vertical EQ band (design §7.7): dB readout on top, center-origin fill rail,
// glowing white thumb, frequency label below, snap-to-zero detent. Self
// contained with vertical-drag handling; emits gain changes.

#include "Theme/DesignTokens.h"
#include "VeyraGui.h"

#include <functional>

namespace veyra::ui {

class EqBandSlider : public juce::Component {
public:
    explicit EqBandSlider(juce::String freqLabel) : freq_(std::move(freqLabel)) {}

    void setPalette(const Palette& p) { palette_ = p; repaint(); }
    void setGainDb(float db) { gain_ = juce::jlimit(-12.0f, 12.0f, db); repaint(); }
    float gainDb() const noexcept { return gain_; }

    // Thumb centre in this component's local coordinates (for the EQ curve).
    juce::Point<float> thumbCentre() const;

    std::function<void(float)> onChange;

    void paint(juce::Graphics&) override;
    void mouseDown(const juce::MouseEvent&) override;
    void mouseDrag(const juce::MouseEvent&) override;

private:
    juce::Rectangle<int> railArea() const;
    void setFromMouseY(float y);

    juce::String freq_;
    float gain_ = 0.0f;
    Palette palette_ = paletteForTheme("midnight");
};

} // namespace veyra::ui
