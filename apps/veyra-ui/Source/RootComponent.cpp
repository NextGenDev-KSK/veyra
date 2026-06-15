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

    // Connect to the service. The client notifies from its own thread; hop to
    // the message thread (guarded by a SafePointer) before touching components.
    home_.attachService(&client_);
    client_.start([safe = juce::Component::SafePointer<RootComponent>(this)]
    {
        juce::MessageManager::callAsync([safe]
        {
            if (safe != nullptr)
                safe->home_.refreshFromService();
        });
    });
}

RootComponent::~RootComponent()
{
    client_.stop(); // join the background thread before the rest tears down
    setLookAndFeel(nullptr);
}

void RootComponent::resized()
{
    home_.setBounds(getLocalBounds());
}

} // namespace veyra::ui
