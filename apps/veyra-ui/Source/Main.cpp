// Veyra Sounds UI — JUCE application entry point (Phase 4a).
//
// Hosts the RootComponent (glass background + Foundations gallery + live theme
// switching). The Phase 1 Win32 shell is replaced; the service ServiceClient is
// reintroduced (refactored) on the Home screen in 4b.

#include "RootComponent.h"
#include "Theme/Fonts.h"
#include "VeyraGui.h"

#include "veyra/version.h"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <dwmapi.h>

namespace {

// Windows 11 (build 22621+) acrylic system backdrop: a live, blurred view of
// whatever is behind the window. Older builds ignore the attributes (no-op), so
// the window simply stays solid. Requires the window to be non-opaque so the
// backdrop shows through the translucent base fill.
void enableAcrylicBackdrop(HWND hwnd)
{
    constexpr DWORD kImmersiveDarkMode = 20; // DWMWA_USE_IMMERSIVE_DARK_MODE
    constexpr DWORD kSystemBackdropType = 38; // DWMWA_SYSTEMBACKDROP_TYPE
    constexpr int   kTransientWindow = 3;     // DWMSBT_TRANSIENTWINDOW (acrylic)

    BOOL dark = TRUE;
    DwmSetWindowAttribute(hwnd, kImmersiveDarkMode, &dark, sizeof(dark));
    int backdrop = kTransientWindow;
    DwmSetWindowAttribute(hwnd, kSystemBackdropType, &backdrop, sizeof(backdrop));
}

class MainWindow : public juce::DocumentWindow {
public:
    MainWindow()
        : juce::DocumentWindow("Veyra Sounds",
                               juce::Colours::transparentBlack, // let the acrylic show
                               juce::DocumentWindow::allButtons)
    {
        // Borderless: our TopBar is the custom glass title bar.
        setUsingNativeTitleBar(false);
        setTitleBarHeight(0);
        setBackgroundColour(juce::Colours::transparentBlack);
        setContentOwned(new veyra::ui::RootComponent(), true);
        setResizable(true, false);
        setResizeLimits(960, 620, 4000, 3000);
        centreWithSize(1180, 760);
        setVisible(true);

        if (auto* peer = getPeer())
            enableAcrylicBackdrop(static_cast<HWND>(peer->getNativeHandle()));
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
