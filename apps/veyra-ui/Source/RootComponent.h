#pragma once

// Application shell: owns the LookAndFeel, theme manager, glass background, top
// bar and sidebar (shared chrome), and the routable content screens. Routes
// sidebar navigation, owns the single working Config, and bridges the service
// client (connection LED + config sync + parameter push).

#include "Graphics/GlassBackground.h"
#include "Home/Sidebar.h"
#include "Home/TopBar.h"
#include "MiniWindow.h"
#include "Screens/HomeScreen.h"
#include "Screens/PlaceholderScreen.h"
#include "Screens/PresetsScreen.h"
#include "Screens/SettingsScreen.h"
#include "ServiceClient.h"
#include "Theme/ThemeManager.h"
#include "Theme/VeyraLookAndFeel.h"
#include "TrayIcon.h"
#include "VeyraGui.h"

#include "veyra/Config.h"

#include <memory>

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

    void setMasterEnabled(bool on); // sync top bar + mini, push
    void setMasterVolume(double gain);
    void enterMiniMode();
    void enterFullMode();

    void saveCurrentAsPreset(const juce::String& name);
    void exportPreset(const juce::String& uuid);
    void importPreset();

    VeyraLookAndFeel laf_;
    ThemeManager     themeManager_;
    GlassBackground  background_;
    TopBar           topBar_;
    Sidebar          sidebar_;

    HomeScreen        home_;
    PresetsScreen     presets_;
    SettingsScreen    settings_;
    PlaceholderScreen placeholder_;
    juce::Component*  current_ = nullptr;

    std::unique_ptr<MiniWindow>      mini_;
    TrayIcon                         tray_;
    std::unique_ptr<juce::FileChooser> chooser_;

    veyra::Config working_;

    ServiceClient client_; // declared last -> destroyed first (thread joins early)
};

} // namespace veyra::ui
