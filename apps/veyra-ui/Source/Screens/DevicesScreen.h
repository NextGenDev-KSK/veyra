#pragma once

// Devices screen: lists system output/input endpoints and provides the Preferred
// Output Device selector for the APO path. The Audio Bridge (WASAPI loopback
// compatibility mode for Bluetooth endpoints that reject APOs) is an advanced
// secondary option, not the primary workflow.

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
    class CardGrid; // scrollable output/input card area

    std::unique_ptr<BridgeCard> card_;
    std::unique_ptr<CardGrid>   grid_;
    juce::Viewport              viewport_; // scrolls the cards if they overflow

    Palette          palette_  = paletteForTheme("midnight");
};

} // namespace veyra::ui
