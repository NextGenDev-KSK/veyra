#include "Components/ToggleSwitch.h"

namespace veyra::ui {

ToggleSwitch::ToggleSwitch() : juce::Button({})
{
    setClickingTogglesState(true);
    setSize(36, 20);
}

void ToggleSwitch::paintButton(juce::Graphics& g, bool over, bool)
{
    // Center a 36x20 track within whatever bounds we were given.
    juce::Rectangle<float> track(0.0f, 0.0f, 36.0f, 20.0f);
    track.setCentre(getLocalBounds().toFloat().getCentre());

    const bool on = getToggleState();
    juce::Colour trackColour = on ? palette_.accentPrimary
                                   : juce::Colour(60, 62, 80).withAlpha(0.6f);
    if (over)
        trackColour = trackColour.brighter(0.05f);

    g.setColour(trackColour);
    g.fillRoundedRectangle(track, track.getHeight() * 0.5f);

    const float thumbD = track.getHeight() - 4.0f;
    const float tx = on ? track.getRight() - thumbD - 2.0f : track.getX() + 2.0f;
    g.setColour(juce::Colours::white);
    g.fillEllipse(tx, track.getY() + 2.0f, thumbD, thumbD);
}

} // namespace veyra::ui
