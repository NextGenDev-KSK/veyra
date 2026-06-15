#pragma once

// The Home screen content: visualizer + 10-band equalizer + 6 effect knobs +
// More Effects. Chrome (background, top bar, sidebar) is owned by the shell
// (RootComponent); this is just the routable content area. Knob/EQ changes are
// reported via onEnhancementChanged and reflected back via applyEnhancement.

#include "Components/GlassPanel.h"
#include "Components/Knob.h"
#include "Home/EqualizerCard.h"
#include "Home/VisualizerCard.h"
#include "Theme/DesignTokens.h"
#include "VeyraGui.h"

#include "veyra/Config.h"

#include <array>
#include <functional>
#include <memory>

namespace veyra::ui {

class GlassBackground;

class HomeScreen : public juce::Component {
public:
    HomeScreen();

    void setPalette(const Palette& p);
    void attachBackdrop(GlassBackground* bg);
    void resized() override;

    // User changed an enhancement param (knob or EQ band).
    std::function<void(const EnhancementConfig&)> onEnhancementChanged;

    // Apply enhancement state from config without firing the callback.
    void applyEnhancement(const EnhancementConfig& e);
    const EnhancementConfig& enhancement() const noexcept { return enh_; }

    void setReduceMotion(bool reduce) { viz_.setReduceMotion(reduce); }

private:
    void onKnobChanged(int index, double v01);
    void seedFromControls();

    VisualizerCard viz_;
    EqualizerCard eq_;

    static constexpr int kKnobs = 6;
    std::array<std::unique_ptr<GlassPanel>, kKnobs> knobCards_;
    std::array<std::unique_ptr<Knob>, kKnobs> knobs_;
    std::unique_ptr<GlassPanel> moreCard_;

    EnhancementConfig enh_;
};

} // namespace veyra::ui
