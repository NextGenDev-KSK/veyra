#include "RootComponent.h"

namespace veyra::ui {

RootComponent::RootComponent()
{
    setLookAndFeel(&laf_);

    const auto& p = themeManager_.palette();
    laf_.setPalette(p);
    home_.setPalette(p);

    addAndMakeVisible(home_);
    setSize(1180, 760);
}

RootComponent::~RootComponent()
{
    setLookAndFeel(nullptr);
}

void RootComponent::resized()
{
    home_.setBounds(getLocalBounds());
}

} // namespace veyra::ui
