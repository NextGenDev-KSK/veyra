#include "Screens/OnboardingOverlay.h"

#include "Theme/Fonts.h"

namespace veyra::ui {

namespace {
struct Step { const char* title; const char* body; };
const Step kStepContent[] = {
    {"Welcome to Veyra Sounds",
     "A free, system-wide audio enhancer. EQ, bass, spatial, loudness and more — "
     "all live, on whatever you're listening to."},
    {"Make it yours",
     "Pick a theme in Settings and dial in the glass. Tweak the 10-band EQ and the "
     "effect knobs on Home, or start from a preset."},
    {"Hear it anywhere",
     "Open Devices to route audio through Veyra to any output — including Bluetooth — "
     "with the Audio Bridge. No driver required."},
    {"Quick keys",
     "Global hotkeys: Ctrl+Alt+M mute, Ctrl+Alt+Up/Down volume, "
     "Ctrl+Alt+Left/Right cycle presets, Ctrl+Alt+N mini mode."},
    {"Your setup",
     "Detecting your devices…"}, // body replaced by setDetectedSetup()
};
} // namespace

OnboardingOverlay::OnboardingOverlay()
{
    setInterceptsMouseClicks(true, true); // eat clicks meant for the app behind

    for (auto* b : {&back_, &next_, &skip_})
        addAndMakeVisible(b);

    next_.onClick = [this] { go(+1); };
    back_.onClick = [this] { go(-1); };
    skip_.onClick = [this] { finish(); };
}

void OnboardingOverlay::setPalette(const Palette& p)
{
    // The TextButtons are themed by the global VeyraLookAndFeel; just repaint.
    palette_ = p;
    repaint();
}

void OnboardingOverlay::setDetectedSetup(juce::String body)
{
    setupBody_ = std::move(body);
    repaint();
}

void OnboardingOverlay::go(int delta)
{
    const int s = step_ + delta;
    if (s < 0)
        return;
    if (s >= kSteps)
    {
        finish();
        return;
    }
    step_ = s;
    resized();
    repaint();
}

void OnboardingOverlay::finish()
{
    if (onFinished)
        onFinished();
}

juce::Rectangle<int> OnboardingOverlay::cardBounds() const
{
    return juce::Rectangle<int>(0, 0, 520, 320).withCentre(getLocalBounds().getCentre());
}

void OnboardingOverlay::resized()
{
    auto card = cardBounds();
    auto footer = card.reduced(28).removeFromBottom(36);
    skip_.setBounds(footer.removeFromLeft(80));
    next_.setBounds(footer.removeFromRight(110));
    footer.removeFromRight(10);
    back_.setBounds(footer.removeFromRight(90));
    back_.setVisible(step_ > 0);
    next_.setButtonText(step_ == kSteps - 1 ? "Finish" : "Next");
}

void OnboardingOverlay::paint(juce::Graphics& g)
{
    // Scrim.
    g.fillAll(palette_.bgModalScrim.withAlpha(0.72f));

    // Card.
    const auto card = cardBounds().toFloat();
    g.setColour(palette_.bgGlassElevated);
    g.fillRoundedRectangle(card, 18.0f);
    g.setColour(palette_.strokeActive);
    g.drawRoundedRectangle(card.reduced(0.5f), 18.0f, 1.0f);

    auto inner = card.toNearestInt().reduced(28);
    inner.removeFromBottom(44); // footer

    g.setColour(palette_.accentSecondary);
    g.setFont(fonts::mono(11.0f, true));
    g.drawText("STEP " + juce::String(step_ + 1) + " / " + juce::String(kSteps),
               inner.removeFromTop(16), juce::Justification::centredLeft, false);
    inner.removeFromTop(8);

    g.setColour(palette_.textPrimary);
    g.setFont(fonts::display(24.0f));
    g.drawText(kStepContent[step_].title, inner.removeFromTop(34), juce::Justification::centredLeft, false);
    inner.removeFromTop(8);

    g.setColour(palette_.textSecondary);
    g.setFont(fonts::body(15.0f));
    const juce::String body = (step_ == kSteps - 1 && setupBody_.isNotEmpty())
                                  ? setupBody_ : juce::String(kStepContent[step_].body);
    g.drawFittedText(body, inner, juce::Justification::topLeft, 6);

    // Step dots.
    const float dy = card.getBottom() - 30.0f;
    const float cx = card.getCentreX();
    for (int i = 0; i < kSteps; ++i)
    {
        g.setColour(i == step_ ? palette_.accentPrimary : palette_.strokeHover);
        g.fillEllipse(cx - (kSteps * 7.0f) * 0.5f + i * 14.0f, dy, 7.0f, 7.0f);
    }
}

} // namespace veyra::ui
