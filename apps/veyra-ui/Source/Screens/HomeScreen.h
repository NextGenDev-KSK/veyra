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

#include "veyra/Config.h"

#include <array>
#include <memory>

namespace veyra::ui {

class ServiceClient;

class HomeScreen : public juce::Component {
public:
    HomeScreen();

    void setPalette(const Palette& p);
    void resized() override;

    // Connect the screen's controls to the service. After this, control changes
    // are pushed to the service and refreshFromService() syncs the UI back.
    void attachService(ServiceClient* client);

    // Called on the message thread when the service connection state or config
    // changes; updates the connection dot and (on connect) the control values.
    void refreshFromService();

private:
    void onKnobChanged(int index, double v01); // map a knob to its DSP param
    void applyConfig(const veyra::Config& c);  // push config -> control state
    void seedConfigFromControls();             // initial working_ from UI defaults
    void pushConfig();                         // working_ -> service

    GlassBackground background_;
    TopBar topBar_;
    Sidebar sidebar_;
    VisualizerCard viz_;
    EqualizerCard eq_;

    static constexpr int kKnobs = 6;
    std::array<std::unique_ptr<GlassPanel>, kKnobs> knobCards_;
    std::array<std::unique_ptr<Knob>, kKnobs> knobs_;
    std::unique_ptr<GlassPanel> moreCard_;

    ServiceClient* client_ = nullptr;
    veyra::Config  working_;
};

} // namespace veyra::ui
