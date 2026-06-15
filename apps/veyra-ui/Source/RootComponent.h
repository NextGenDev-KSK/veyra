#pragma once

// Top-level content component: owns the LookAndFeel, theme manager, glass
// background and the current view (the Foundations gallery in 4a). Applies the
// active palette across everything and repaints on theme change.

#include "Graphics/GlassBackground.h"
#include "Screens/GalleryComponent.h"
#include "Theme/ThemeManager.h"
#include "Theme/VeyraLookAndFeel.h"
#include "VeyraGui.h"

namespace veyra::ui {

class RootComponent : public juce::Component, private juce::ChangeListener {
public:
    RootComponent();
    ~RootComponent() override;

    void resized() override;

private:
    void changeListenerCallback(juce::ChangeBroadcaster*) override;
    void applyPalette();

    VeyraLookAndFeel laf_;
    ThemeManager     themeManager_;
    GlassBackground  background_;
    GalleryComponent gallery_;
};

} // namespace veyra::ui
