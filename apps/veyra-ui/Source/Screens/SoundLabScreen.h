#pragma once

// Sound Lab (matches Reference theme/sounds.png): a top tab bar of the 7 test
// tools over a central card. Each tool drives the SoundLabEngine to emit a real
// test signal (sine / sweep / noise / per-channel / polarity) or read the mic.

#include "Components/SegmentedControl.h"
#include "SoundLabEngine.h"
#include "Theme/DesignTokens.h"
#include "VeyraGui.h"

#include <memory>

namespace veyra::ui {

class GlassBackground;

class SoundLabScreen : public juce::Component, private juce::Timer {
public:
    SoundLabScreen();
    ~SoundLabScreen() override;

    void setPalette(const Palette& p);
    void attachBackdrop(GlassBackground* bg);
    void resized() override;
    void paint(juce::Graphics& g) override;
    void visibilityChanged() override;

private:
    class LabCard;
    void selectTool(int i);
    void startCurrentTool();
    void playTone(double hz);
    void playChannel(veyra::dsp::TestChannel ch);
    void playPhase(bool invert);
    void timerCallback() override;

    static constexpr int kTools = 7;
    int          tool_ = 0;
    bool         engineActiveForHearing_ = false;
    Palette      palette_ = paletteForTheme("midnight");

    SegmentedControl         tabs_;
    std::unique_ptr<LabCard> card_;

    juce::TextButton start_, stop_, chL_, chR_, chBoth_, phIn_, phInv_;
    SegmentedControl noiseType_;
    juce::Slider     freq_;

    SoundLabEngine engine_;
};

} // namespace veyra::ui
