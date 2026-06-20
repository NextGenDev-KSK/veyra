#pragma once

// Devices screen: lists the system output endpoints and hosts the Audio Bridge
// router (enable + source/target pickers) so the user can process any app's
// audio to any output — including Bluetooth — without editing config.json.

#include "Theme/DesignTokens.h"
#include "VeyraGui.h"

#include "veyra/Config.h"

#include <functional>
#include <memory>

namespace veyra::ui {

class GlassBackground;

class DevicesScreen : public juce::Component {
public:
    DevicesScreen();
    ~DevicesScreen() override;

    void setPalette(const Palette& p);
    void attachBackdrop(GlassBackground* bg);
    void resized() override;

    void refreshDevices();                          // re-enumerate endpoints
    void setBridge(const veyra::BridgeConfig& b);   // reflect state, no callback

    std::function<void(const veyra::BridgeConfig&)> onBridgeChanged;

private:
    class BridgeCard;
    std::unique_ptr<BridgeCard> card_;
};

} // namespace veyra::ui
