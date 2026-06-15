#pragma once

// Equalizer glass card: 10 EQ band sliders, Graphic/Parametric segmented toggle,
// Show curve / Show spectrum toggles, a Reset button, and a response-curve
// overlay traced through the band thumbs.

#include "Components/EqBandSlider.h"
#include "Components/GlassPanel.h"
#include "Components/SegmentedControl.h"
#include "Components/ToggleSwitch.h"
#include "VeyraGui.h"

#include <array>
#include <memory>

namespace veyra::ui {

class EqualizerCard : public GlassPanel {
public:
    EqualizerCard();

    void setPalette(const Palette& p) override;
    void resized() override;
    void paintContent(juce::Graphics&) override;

    std::function<void(int, float)> onBandChanged; // (band index, dB)

    // Set a band's gain from config without firing onBandChanged.
    void setBandGain(int index, float db);
    float bandGain(int index) const;

    static constexpr int kBands = 10;

private:
    std::array<std::unique_ptr<EqBandSlider>, kBands> bands_;
    SegmentedControl modeToggle_;
    ToggleSwitch showCurve_, showSpectrum_;
    juce::TextButton reset_{"Reset"};
};

} // namespace veyra::ui
