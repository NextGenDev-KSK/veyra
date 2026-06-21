#pragma once

// Visualizer glass card: spectrum bars + L/R VU/peak meters fed live from the
// service's loopback analyzer (idle animation when the service isn't running),
// plus a mode dropdown and fullscreen button.

#include "Components/GlassPanel.h"
#include "Components/IconButton.h"
#include "Graphics/Icons.h"
#include "VeyraGui.h"

#include <array>

namespace veyra::ui {

class VisualizerCard : public GlassPanel, private juce::Timer {
public:
    VisualizerCard();

    void setPalette(const Palette& p) override;
    void resized() override;
    void paintContent(juce::Graphics&) override;

    // Reduce-motion (Settings→Appearance): freeze the animation to a static frame.
    void setReduceMotion(bool reduce);

    // Live metering from the service's loopback analyzer. While fresh frames keep
    // arriving the card shows real audio; otherwise it falls back to idle motion.
    void setLiveFrame(const float* bars, int n,
                      float vuL, float vuR, float peakL, float peakR, bool clip);

private:
    void timerCallback() override;
    void drawSpectrum(juce::Graphics& g, juce::Rectangle<float> area);

    static constexpr int kBars = 48;
    int modeIndex_ = 0;                   // 0 Bars .. 7 3D Bars (matches mode_ items)
    std::array<float, kBars> bars_{};    // normalised 0..1
    std::array<float, kBars> targets_{}; // normalised 0..1
    std::array<float, kBars> caps_{};    // falling caps (Monstercat)
    float vuL_ = 0.4f, vuR_ = 0.3f;
    float peakL_ = 0.4f, peakR_ = 0.3f;
    bool clip_ = false;
    int frame_ = 0;
    int liveTtl_ = 0; // frames remaining to treat as live before idle fallback
    juce::Random rng_;

    juce::ComboBox mode_;
    IconButton fullscreen_{icons::fullscreen};
};

} // namespace veyra::ui
