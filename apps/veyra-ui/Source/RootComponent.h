#pragma once

// Application shell: owns the LookAndFeel, theme manager, glass background, top
// bar and sidebar (shared chrome), and the routable content screens. Routes
// sidebar navigation, owns the single working Config, and bridges the service
// client (connection LED + config sync + parameter push).

#include "Graphics/GlassBackground.h"
#include "Home/Sidebar.h"
#include "Home/TopBar.h"
#include "Screens/HomeScreen.h"
#include "Screens/PlaceholderScreen.h"
#include "Screens/SettingsScreen.h"
#include "ServiceClient.h"
#include "Theme/ThemeManager.h"
#include "Theme/VeyraLookAndFeel.h"
#include "VeyraGui.h"

#include "veyra/Config.h"

namespace veyra::ui {

class RootComponent : public juce::Component, private juce::ChangeListener {
public:
    RootComponent();
    ~RootComponent() override;

    void resized() override;

private:
    void changeListenerCallback(juce::ChangeBroadcaster*) override; // theme change
    void showScreen(int navIndex);
    void applyPalette();
    void applyConfig(const veyra::Config& c);
    void pushConfig();
    void refreshFromService();

    VeyraLookAndFeel laf_;
    ThemeManager     themeManager_;
    GlassBackground  background_;
    TopBar           topBar_;
    Sidebar          sidebar_;

    HomeScreen        home_;
    SettingsScreen    settings_;
    PlaceholderScreen placeholder_;
    juce::Component*  current_ = nullptr;

    veyra::Config working_;

    ServiceClient client_; // declared last -> destroyed first (thread joins early)
};

} // namespace veyra::ui
