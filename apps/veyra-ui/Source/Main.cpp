// Veyra Sounds UI — JUCE application entry point (Phase 4a).
//
// Hosts the RootComponent (glass background + Foundations gallery + live theme
// switching). The Phase 1 Win32 shell is replaced; the service ServiceClient is
// reintroduced (refactored) on the Home screen in 4b.

#include "RootComponent.h"
#include "Theme/Fonts.h"
#include "VeyraGui.h"

#include "veyra/version.h"

namespace {

class MainWindow : public juce::DocumentWindow {
public:
    MainWindow()
        : juce::DocumentWindow("Veyra Sounds",
                               juce::Colour(0xff0A0B10),
                               juce::DocumentWindow::allButtons)
    {
        // Borderless: our TopBar is the custom glass title bar.
        setUsingNativeTitleBar(false);
        setTitleBarHeight(0);
        setContentOwned(new veyra::ui::RootComponent(), true);
        setResizable(true, false);
        setResizeLimits(960, 620, 4000, 3000);
        centreWithSize(1180, 760);
        setVisible(true);
    }

    void closeButtonPressed() override
    {
        juce::JUCEApplication::getInstance()->systemRequestedQuit();
    }
};

class VeyraApplication : public juce::JUCEApplication {
public:
    const juce::String getApplicationName() override { return "Veyra Sounds"; }
    const juce::String getApplicationVersion() override { return veyra::kVersionString; }
    bool moreThanOneInstanceAllowed() override { return false; }

    void initialise(const juce::String&) override
    {
        veyra::ui::fonts::initialise();
        mainWindow_ = std::make_unique<MainWindow>();
    }

    void shutdown() override { mainWindow_ = nullptr; }

    void systemRequestedQuit() override { quit(); }

private:
    std::unique_ptr<MainWindow> mainWindow_;
};

} // namespace

START_JUCE_APPLICATION(VeyraApplication)
