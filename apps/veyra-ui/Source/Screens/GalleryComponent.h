#pragma once

// Phase 4a "Foundations" view: renders the component library on glass cards so
// the design language can be verified by running the built artifact, plus a
// theme picker that exercises live theme switching. Replaced by the real Home
// screen in 4b (components carry over).

#include "Components/Knob.h"
#include "Components/SegmentedControl.h"
#include "Components/ToggleSwitch.h"
#include "Graphics/GlassCard.h"
#include "Theme/DesignTokens.h"
#include "VeyraGui.h"

#include <functional>

namespace veyra::ui {

class GalleryComponent : public juce::Component {
public:
    GalleryComponent();

    void setPalette(const Palette& p);
    std::function<void(juce::String)> onThemeSelected;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    Palette palette_ = paletteForTheme("midnight");

    GlassCard buttonsCard_, controlsCard_, themesCard_;

    juce::TextButton primaryButton_{"Primary"};
    juce::TextButton ghostButton_{"Ghost"};
    juce::TextButton dangerButton_{"Delete"};
    ToggleSwitch toggle_;

    juce::Slider hslider_;
    Knob knob_;
    SegmentedControl segmented_;

    juce::OwnedArray<juce::TextButton> themeButtons_;
};

} // namespace veyra::ui
