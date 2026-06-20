#pragma once

// Apps screen: edit per-app rules (match a foreground app -> a preset). Reads
// and writes %APPDATA%\Veyra\app_rules.json directly — the same file the
// service and the UI's AppRuleWatcher consume, so saved rules take effect on the
// next focus change with no extra IPC.

#include "Theme/DesignTokens.h"
#include "VeyraGui.h"

#include "veyra/Preset.h"

#include <memory>
#include <vector>

namespace veyra::ui {

class GlassBackground;

class AppsScreen : public juce::Component {
public:
    AppsScreen();
    ~AppsScreen() override;

    void setPalette(const Palette& p);
    void attachBackdrop(GlassBackground* bg);
    void resized() override;

    void setPresets(const std::vector<veyra::Preset>& presets); // populate the pickers

private:
    class Card;
    std::unique_ptr<Card> card_;
};

} // namespace veyra::ui
