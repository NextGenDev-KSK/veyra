#include "RootComponent.h"

namespace veyra::ui {

RootComponent::RootComponent()
{
    setLookAndFeel(&laf_);

    addAndMakeVisible(background_);
    addAndMakeVisible(gallery_);

    gallery_.onThemeSelected = [this](juce::String id) { themeManager_.setTheme(id); };
    themeManager_.addChangeListener(this);

    applyPalette();
    setSize(1180, 760);
}

RootComponent::~RootComponent()
{
    themeManager_.removeChangeListener(this);
    setLookAndFeel(nullptr);
}

void RootComponent::applyPalette()
{
    const auto& p = themeManager_.palette();
    laf_.setPalette(p);
    background_.setPalette(p);
    gallery_.setPalette(p);
    repaint();
}

void RootComponent::changeListenerCallback(juce::ChangeBroadcaster*)
{
    applyPalette();
}

void RootComponent::resized()
{
    background_.setBounds(getLocalBounds());
    gallery_.setBounds(getLocalBounds());
}

} // namespace veyra::ui
