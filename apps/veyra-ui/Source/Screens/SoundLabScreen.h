#pragma once

// Sound Lab screen: the listening/loudness tools — Night Mode compression,
// EBU R128 Loudness Match (+ target), and the Sleep Timer with fade-out. Drives
// config.loudness.

#include "Theme/DesignTokens.h"
#include "VeyraGui.h"

#include "veyra/Config.h"

#include <functional>
#include <memory>

namespace veyra::ui {

class GlassBackground;

class SoundLabScreen : public juce::Component {
public:
    SoundLabScreen();
    ~SoundLabScreen() override;

    void setPalette(const Palette& p);
    void attachBackdrop(GlassBackground* bg);
    void resized() override;

    void setLoudness(const veyra::LoudnessConfig& l); // reflect state, no callback
    std::function<void(const veyra::LoudnessConfig&)> onLoudnessChanged;

private:
    class Card;
    std::unique_ptr<Card> card_;
};

} // namespace veyra::ui
