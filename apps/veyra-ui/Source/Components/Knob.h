#pragma once

// Effect knob (design §7.8): 270° arc (225°→495°), accent fill, inner glass
// circle, indicator line, LED dot, label above, numeric value below. Self
// contained (own value + vertical-drag handling) so the gallery can use it
// directly; wired to DSP params on the Home screen in 4b.

#include "Theme/DesignTokens.h"
#include "VeyraGui.h"

#include <functional>

namespace veyra::ui {

class Knob : public juce::Component {
public:
    Knob() = default;

    void setPalette(const Palette& p) { palette_ = p; repaint(); }
    void setLabel(juce::String text) { label_ = std::move(text); repaint(); }
    void setValueText(juce::String text) { valueText_ = std::move(text); repaint(); }
    void setValue(double v01) { value_ = juce::jlimit(0.0, 1.0, v01); repaint(); }
    double getValue() const noexcept { return value_; }

    // Dim the knob (off look) when its value is zero — used for Reverb/Compression.
    void setDimWhenZero(bool b) { dimWhenZero_ = b; repaint(); }
    // Arc turns to the warning/danger colour past this normalised value (e.g.
    // Volume Gain beyond 100%). Default > 1 disables it.
    void setDangerThreshold(float t) { dangerThreshold_ = t; repaint(); }

    std::function<void(double)> onChange;

    void paint(juce::Graphics&) override;
    void mouseDown(const juce::MouseEvent&) override;
    void mouseDrag(const juce::MouseEvent&) override;

private:
    Palette palette_ = paletteForTheme("midnight");
    juce::String label_, valueText_;
    double value_ = 0.0;
    double dragStartValue_ = 0.0;
    bool dimWhenZero_ = false;
    float dangerThreshold_ = 2.0f; // > 1 => no danger zone
};

} // namespace veyra::ui
