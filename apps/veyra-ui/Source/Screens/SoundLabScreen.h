#pragma once

// Sound Lab (matches Reference theme/sounds.png): a top tab bar of the 7 test
// tools over a central card. Each tool drives the SoundLabEngine to emit a real
// test signal (sine / sweep / noise / per-channel / polarity) or read the mic.

#include "Components/SegmentedControl.h"
#include "SoundLabEngine.h"
#include "Theme/DesignTokens.h"
#include "VeyraGui.h"

#include "veyra/HearingTest.h" // ParametricBand + personalizeFromHearingTest

#include <array>
#include <functional>
#include <memory>
#include <vector>

namespace veyra::ui {

class GlassBackground;

class SoundLabScreen : public juce::Component, private juce::Timer {
public:
    SoundLabScreen();
    ~SoundLabScreen() override;

    void setPalette(const Palette& p);
    void attachBackdrop(GlassBackground* bg);

    // Fired when the hearing-test wizard produces a personalized correction; the
    // shell applies the bands as the parametric EQ.
    std::function<void(const std::vector<veyra::ParametricBand>&)> onPersonalized;

    void resized() override;
    void paint(juce::Graphics& g) override;
    void visibilityChanged() override;

private:
    class LabCard;
    void selectTool(int i);
    void startCurrentTool();
    void playTone(double hz);
    void playToneAt(double hz, float level);
    void playChannel(veyra::dsp::TestChannel ch);
    void playPhase(bool invert);
    void timerCallback() override;

    // Hearing-test personalization wizard (calibration-free, self-relative).
    void hpStartWizard();
    void hpRecordAndAdvance();
    void hpFinish();
    void hpUpdateCard();
    bool hpActive() const { return wizStep_ >= 0; }

    static constexpr int kTools = 7;
    static constexpr int kHpSteps = 6;
    int          tool_ = 0;
    bool         engineActiveForHearing_ = false;
    int          wizStep_ = -1; // -1 = not running, else 0..kHpSteps-1
    std::array<float, kHpSteps> wizThresh_{};
    Palette      palette_ = paletteForTheme("midnight");

    SegmentedControl         tabs_;
    std::unique_ptr<LabCard> card_;

    juce::TextButton start_, stop_, chL_, chR_, chBoth_, phIn_, phInv_;
    juce::TextButton hpStart_, hpNext_;
    SegmentedControl noiseType_;
    juce::Slider     freq_, hpLevel_;

    SoundLabEngine engine_;
};

} // namespace veyra::ui
