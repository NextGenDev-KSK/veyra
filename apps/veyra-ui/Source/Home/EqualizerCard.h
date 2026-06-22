#pragma once

// Equalizer glass card: 10 EQ band sliders, Graphic/Parametric segmented toggle,
// Show curve / Show spectrum toggles, a Reset button, and a response-curve
// overlay traced through the band thumbs.

#include "Components/EqBandSlider.h"
#include "Components/GlassPanel.h"
#include "Components/ParametricEqEditor.h"
#include "Components/SegmentedControl.h"
#include "Components/ToggleSwitch.h"
#include "VeyraGui.h"

#include "veyra/AutoEq.h"
#include "veyra/Config.h"

#include <array>
#include <memory>
#include <vector>

namespace veyra::ui {

class EqualizerCard : public GlassPanel {
public:
    EqualizerCard();

    void setPalette(const Palette& p) override;
    void resized() override;
    void paintContent(juce::Graphics&) override;

    std::function<void(int, float)> onBandChanged; // (band index, dB)
    std::function<void(bool)>       onModeChanged; // true = parametric
    std::function<void(const std::vector<veyra::ParametricBand>&)> onParametricChanged;
    std::function<void(const std::vector<veyra::ParametricBand>&)> onAutoEqSelected; // headphone correction

    void setAutoEqProfiles(std::vector<veyra::AutoEqProfile> profiles);

    // Set a band's gain from config without firing onBandChanged.
    void setBandGain(int index, float db);
    float bandGain(int index) const;
    void setMode(bool parametric); // reflect graphic/parametric, no callback
    void setParametricBands(std::vector<veyra::ParametricBand> bands);
    void setSpectrum(const float* bars, int n); // live FFT underlay for the curve

    static constexpr int kBands = 10;

private:
    std::array<std::unique_ptr<EqBandSlider>, kBands> bands_;
    std::unique_ptr<ParametricEqEditor> paramEditor_;
    bool parametric_ = false;
    SegmentedControl modeToggle_;
    ToggleSwitch showCurve_, showSpectrum_;
    juce::TextButton reset_{"Reset"};
    juce::TextButton autoEqBtn_{"AutoEQ"};
    std::vector<veyra::AutoEqProfile> autoEq_;
};

} // namespace veyra::ui
