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
    void paint(juce::Graphics& g) override;            // title + sub-nav
    void mouseDown(const juce::MouseEvent& e) override; // sub-nav selection

    // Appearance interactions.
    std::function<void(const juce::String&)> onThemeSelected;   // theme id
    std::function<void(double)>              onOpacity;         // 0..1
    std::function<void(int)>                 onBackgroundMode;  // 0 blobs, 1 solid, 2 image
    std::function<void(bool)>                onReduceMotion;

    // Microphone (voice chain) interactions.
    std::function<void(const veyra::VoiceConfig&)> onMicChanged;

    // Spatial (headphone) interactions.
    std::function<void(const veyra::SpatialConfig&)> onSpatialChanged;

    // Loudness (Night Mode + Sleep Timer) interactions.
    std::function<void(const veyra::LoudnessConfig&)> onLoudnessChanged;

    // Audio Engine interactions.
    std::function<void(const veyra::AudioEngineConfig&)> onAudioEngineChanged;

    // About: reset all settings to defaults.
    std::function<void()> onResetSettings;

    // Reflect current state without firing callbacks.
    void setCurrentTheme(const juce::String& id);
    void setAppearance(double opacity, int backgroundMode, bool reduceMotion);
    void setMicConfig(const veyra::VoiceConfig& voice);
    void setSpatialConfig(const veyra::SpatialConfig& spatial);
    void setLoudnessConfig(const veyra::LoudnessConfig& loud);
    void setAudioEngineConfig(const veyra::AudioEngineConfig& engine);
    void setServiceStatus(bool connected, juce::String version);

private:
    class AppearanceCard;
    class AudioEngineCard;
    class MicrophoneCard;
    class SpatialCard;
    class LoudnessCard;
    class UpdatesCard;
    class AboutCard;

    void setSection(int i);
    juce::Rectangle<int> navItemBounds(int i) const;
    juce::Component* cardForSection(int i) const;

    static constexpr int kSections = 7; // Appearance, Audio Engine, Microphone, Spatial, Loudness, Updates, About
    int section_ = 0;

    std::unique_ptr<AppearanceCard>  appearance_;
    std::unique_ptr<AudioEngineCard> audioEngine_;
    std::unique_ptr<MicrophoneCard>  microphone_;
    std::unique_ptr<SpatialCard>     spatial_;
    std::unique_ptr<LoudnessCard>    loudness_;
    std::unique_ptr<UpdatesCard>     updates_;
    std::unique_ptr<AboutCard>       about_;
    Palette palette_ = paletteForTheme("midnight");
};

} // namespace veyra::ui
