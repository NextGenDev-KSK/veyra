#pragma once

// Settings screen — Phase 4c (Appearance) + Phase 6 (Microphone). Appearance:
// an 11-theme preview grid (live theme switch), a UI opacity slider, a
// background-mode selector, and a reduce-motion toggle. Microphone: the voice
// chain controls. Content only (no chrome); the shell supplies the chrome.

#include "Theme/DesignTokens.h"
#include "VeyraGui.h"

#include "veyra/Config.h"

#include <functional>
#include <memory>

namespace veyra::ui {

class GlassBackground;

class SettingsScreen : public juce::Component {
public:
    SettingsScreen();
    ~SettingsScreen() override;

    void setPalette(const Palette& p);
    void attachBackdrop(GlassBackground* bg);
    void resized() override;

    // Appearance interactions.
    std::function<void(const juce::String&)> onThemeSelected;   // theme id
    std::function<void(double)>              onOpacity;         // 0..1
    std::function<void(int)>                 onBackgroundMode;  // 0 blobs, 1 solid, 2 image
    std::function<void(bool)>                onReduceMotion;

    // Microphone (voice chain) interactions.
    std::function<void(const veyra::VoiceConfig&)> onMicChanged;

    // Spatial (headphone) interactions.
    std::function<void(const veyra::SpatialConfig&)> onSpatialChanged;

    // About: reset all settings to defaults.
    std::function<void()> onResetSettings;

    // Reflect current state without firing callbacks.
    void setCurrentTheme(const juce::String& id);
    void setAppearance(double opacity, int backgroundMode, bool reduceMotion);
    void setMicConfig(const veyra::VoiceConfig& voice);
    void setSpatialConfig(const veyra::SpatialConfig& spatial);
    void setServiceStatus(bool connected, juce::String version);

private:
    class AppearanceCard;
    class MicrophoneCard;
    class SpatialCard;
    class AboutCard;
    std::unique_ptr<AppearanceCard> appearance_;
    std::unique_ptr<MicrophoneCard> microphone_;
    std::unique_ptr<SpatialCard>    spatial_;
    std::unique_ptr<AboutCard>      about_;
};

} // namespace veyra::ui
