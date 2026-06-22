#pragma once

// Devices screen: lists the system output endpoints and hosts the Audio Bridge
// router (enable + source/target pickers) so the user can process any app's
// audio to any output — including Bluetooth — without editing config.json.

#include "AudioDevices.h"
#include "Theme/DesignTokens.h"
#include "VeyraGui.h"

#include "veyra/Config.h"

#include <functional>
#include <memory>
#include <vector>

namespace veyra::ui {

class GlassBackground;

class DevicesScreen : public juce::Component {
public:
    DevicesScreen();
    ~DevicesScreen() override;

    void setPalette(const Palette& p);
    void attachBackdrop(GlassBackground* bg);
    void resized() override;
    void paint(juce::Graphics& g) override; // title + section labels

    void refreshDevices();                          // re-enumerate endpoints
    void setBridge(const veyra::BridgeConfig& b);   // reflect state, no callback

    // Live state shown on the active output / input cards.
    void setActivePreset(juce::String name);
    void setMasterVolume(double gain);
    void setMicProfile(juce::String profile);

    std::function<void(const veyra::BridgeConfig&)> onBridgeChanged;

private:
    class BridgeCard;
    class DeviceCard;
    void rebuildCards();

    std::unique_ptr<BridgeCard> card_;
    std::vector<std::unique_ptr<DeviceCard>> outCards_, inCards_;
    std::vector<OutputDevice> outDevs_, inDevs_; // cached enumeration

    Palette          palette_  = paletteForTheme("midnight");
    GlassBackground* backdrop_ = nullptr;
    juce::String     activePreset_{"Custom"};
    double           masterVol_ = 1.0;
    juce::String     micProfile_{"gaming"};
};

} // namespace veyra::ui
