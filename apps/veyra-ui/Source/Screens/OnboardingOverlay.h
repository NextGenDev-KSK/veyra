#pragma once

// First-run onboarding: a 4-step glass overlay shown over the whole window until
// the user finishes or skips. Emits onFinished so the shell persists
// config.onboardingComplete. Intercepts clicks so the app behind is inert.

#include "Theme/DesignTokens.h"
#include "VeyraGui.h"

#include <functional>

namespace veyra::ui {

class OnboardingOverlay : public juce::Component {
public:
    OnboardingOverlay();

    void setPalette(const Palette& p);
    std::function<void()> onFinished;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    void go(int delta);
    void finish();
    juce::Rectangle<int> cardBounds() const;

    static constexpr int kSteps = 4;
    int     step_ = 0;
    Palette palette_ = paletteForTheme("midnight");

    juce::TextButton back_{"Back"}, next_{"Next"}, skip_{"Skip"};
};

} // namespace veyra::ui
