#pragma once

// Gamer Mode dashboard (matches Reference theme/gamermode.png): a 2x2 grid of
// cards — Sound Tracker, Spatial Audio, Voice & Microphone, Night Mode — plus a
// header master toggle. Each card drives the relevant config block.

#include "Components/ToggleSwitch.h"
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
    void paint(juce::Graphics& g) override;

    void setGamer(const veyra::GamerModeConfig& g);
    void setSpatial(const veyra::SpatialConfig& s);
    void setVoice(const veyra::VoiceConfig& v);
    void setLoudness(const veyra::LoudnessConfig& l);

    std::function<void(const veyra::GamerModeConfig&)> onGamerChanged;
    std::function<void(const veyra::SpatialConfig&)>   onSpatialChanged;
    std::function<void(const veyra::VoiceConfig&)>     onVoiceChanged;
    std::function<void(const veyra::LoudnessConfig&)>  onLoudnessChanged;
    std::function<void()>                              onTestInSoundLab;

private:
    class TrackerCard;
    class SpatialCard;
    class VoiceCard;
    class NightCard;

    std::unique_ptr<TrackerCard> tracker_;
    std::unique_ptr<SpatialCard> spatial_;
    std::unique_ptr<VoiceCard>   voice_;
    std::unique_ptr<NightCard>   night_;

    ToggleSwitch master_;
    Palette      palette_ = paletteForTheme("midnight");
};

} // namespace veyra::ui
