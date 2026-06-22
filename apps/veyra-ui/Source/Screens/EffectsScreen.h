#pragma once

// Effects rack overview — opened from the Home "More Effects" tile. Lists the
// full DSP signal chain with each module's live state (on/off + value) read from
// the working config, so it doubles as an at-a-glance status of the chain.
// Content only; the shell supplies the chrome.

#include "Theme/DesignTokens.h"
#include "VeyraGui.h"

#include "veyra/Config.h"

#include <functional>
#include <memory>

namespace veyra::ui {

class GlassBackground;

class EffectsScreen : public juce::Component {
public:
    EffectsScreen();
    ~EffectsScreen() override;

    void setPalette(const Palette& p);
    void attachBackdrop(GlassBackground* bg);
    void setConfig(const veyra::Config& c);
    void resized() override;

    std::function<void()> onBack;
    std::function<void()> onOpenSoundQuality; // jump to Settings -> Sound Quality

private:
    class RackCard;
    std::unique_ptr<RackCard> card_;
};

} // namespace veyra::ui
