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

    // Reflect current state without firing callbacks.
    void setCurrentTheme(const juce::String& id);
    void setMicConfig(const veyra::VoiceConfig& voice);

private:
    class AppearanceCard;
    class MicrophoneCard;
    std::unique_ptr<AppearanceCard> appearance_;
    std::unique_ptr<MicrophoneCard> microphone_;
};

} // namespace veyra::ui
