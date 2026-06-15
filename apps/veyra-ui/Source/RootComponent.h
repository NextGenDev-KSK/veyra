#pragma once

// Top-level content component: owns the LookAndFeel + theme manager and hosts
// the current screen (Home in 4b). Applies the active palette across it.

#include "Screens/HomeScreen.h"
#include "ServiceClient.h"
#include "Theme/ThemeManager.h"
#include "Theme/VeyraLookAndFeel.h"
#include "VeyraGui.h"

namespace veyra::ui {

class RootComponent : public juce::Component {
public:
    RootComponent();
    ~RootComponent() override;

    void resized() override;

private:
    VeyraLookAndFeel laf_;
    ThemeManager     themeManager_;
    HomeScreen       home_;
    ServiceClient    client_; // declared last -> destroyed first (thread joins early)
};

} // namespace veyra::ui
