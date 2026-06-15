#pragma once

// Visualizer glass card: animated spectrum bars (placeholder data until wired to
// the APO analyzer ring buffer), L/R VU meters, mode dropdown, fullscreen button
// and an FPS badge.

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

private:
    void timerCallback() override;

    static constexpr int kBars = 48;
    std::array<float, kBars> bars_{};
    float vuL_ = 0.4f, vuR_ = 0.3f;
    juce::Random rng_;

    juce::ComboBox mode_;
    IconButton fullscreen_{icons::fullscreen};
};

} // namespace veyra::ui
