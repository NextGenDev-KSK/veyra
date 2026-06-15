#pragma once

// The Home screen: glass background + top bar + sidebar + visualizer card + 10
// band equalizer + 6 effect knobs + More Effects. Matches the approved mockup.

#include "Components/GlassPanel.h"
#include "Components/Knob.h"
#include "Graphics/GlassBackground.h"
#include "Home/EqualizerCard.h"
#include "Home/Sidebar.h"
#include "Home/TopBar.h"
#include "Home/VisualizerCard.h"
#include "Theme/DesignTokens.h"
#include "VeyraGui.h"

#include <array>
#include <memory>

namespace veyra::ui {

class HomeScreen : public juce::Component {
public:
    HomeScreen();

    void setPalette(const Palette& p);
    void resized() override;

private:
    GlassBackground background_;
    TopBar topBar_;
    Sidebar sidebar_;
    VisualizerCard viz_;
    EqualizerCard eq_;

    static constexpr int kKnobs = 6;
    std::array<std::unique_ptr<GlassPanel>, kKnobs> knobCards_;
    std::array<std::unique_ptr<Knob>, kKnobs> knobs_;
    std::unique_ptr<GlassPanel> moreCard_;
};

} // namespace veyra::ui
