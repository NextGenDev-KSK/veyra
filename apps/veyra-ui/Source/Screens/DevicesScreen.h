#pragma once

// Devices screen: lists system output/input endpoints and hosts the Audio Bridge
// controls (enable + capture/playback pickers). On unsigned builds the Bridge is
// the audio path: apps play into a virtual sink, the service loopback-captures
// it, runs the DSP, and renders to the real device. The Preferred Output picker
// serves the signed APO path and applies while the Bridge is off.

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

    void refreshDevices();                            // re-enumerate endpoints
    void setBridge(const veyra::BridgeConfig& b);     // reflect state, no callback
    void setMicBridge(const veyra::MicBridgeConfig&); // reflect state, no callback

    // Live state shown on the active output / input cards.
    void setActivePreset(juce::String name);
    void setMasterVolume(double gain);
    void setMicProfile(juce::String profile);

    std::function<void(const veyra::BridgeConfig&)>    onBridgeChanged;
    std::function<void(const veyra::MicBridgeConfig&)> onMicBridgeChanged;

private:
    class BridgeCard;
    class DeviceCard;
    class CardGrid; // scrollable output/input card area

    std::unique_ptr<BridgeCard> card_;
    std::unique_ptr<CardGrid>   grid_;
    juce::Viewport              viewport_; // scrolls the cards if they overflow

    Palette          palette_  = paletteForTheme("midnight");
};

} // namespace veyra::ui
