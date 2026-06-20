#pragma once

// Gamer Mode screen: drives config.gamerMode (enable, radar style, detection
// sensitivity, which event types to track) and launches the overlay HUD.

#include "Theme/DesignTokens.h"
#include "VeyraGui.h"

#include "veyra/Config.h"

#include <functional>
#include <memory>

namespace veyra::ui {

class GlassBackground;

class GamerModeScreen : public juce::Component {
public:
    GamerModeScreen();
    ~GamerModeScreen() override;

    void setPalette(const Palette& p);
    void attachBackdrop(GlassBackground* bg);
    void resized() override;

    void setGamer(const veyra::GamerModeConfig& g); // reflect state, no callback
    std::function<void(const veyra::GamerModeConfig&)> onGamerChanged;

private:
    class Card;
    std::unique_ptr<Card> card_;
};

} // namespace veyra::ui
