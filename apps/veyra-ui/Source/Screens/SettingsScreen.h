#pragma once

// Settings screen — Phase 4c. Currently the Appearance section: an 11-theme
// preview grid (live theme switch), a UI opacity slider, a background-mode
// selector, and a reduce-motion toggle. Other settings sections land in later
// phases. Content only (no chrome); the shell supplies the top bar + sidebar.

#include "Theme/DesignTokens.h"
#include "VeyraGui.h"

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

    // Reflect current state without firing callbacks.
    void setCurrentTheme(const juce::String& id);

private:
    class AppearanceCard;
    std::unique_ptr<AppearanceCard> appearance_;
};

} // namespace veyra::ui
