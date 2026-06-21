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

// The brand PNG has transparent padding, so as a taskbar icon the mark looks
// small. Trim to the opaque bounds and re-centre on a square canvas so the logo
// fills the icon.
static juce::Image taskbarIcon()
{
    auto img = juce::ImageCache::getFromMemory(BinaryData::Veyra_Icon_square_png,
                                               BinaryData::Veyra_Icon_square_pngSize);
    if (!img.isValid())
        return img;
    img = img.convertedToFormat(juce::Image::ARGB);

    int minX = img.getWidth(), minY = img.getHeight(), maxX = 0, maxY = 0;
    bool any = false;
    {
        const juce::Image::BitmapData bd(img, juce::Image::BitmapData::readOnly);
        for (int y = 0; y < img.getHeight(); ++y)
            for (int x = 0; x < img.getWidth(); ++x)
                if (bd.getPixelColour(x, y).getAlpha() > 16)
                {
                    any = true;
                    minX = juce::jmin(minX, x); minY = juce::jmin(minY, y);
                    maxX = juce::jmax(maxX, x); maxY = juce::jmax(maxY, y);
                }
    }
    if (!any)
        return img;

    const int cw = maxX - minX + 1, ch = maxY - minY + 1;
    const int s = juce::jmax(cw, ch);
    juce::Image square(juce::Image::ARGB, s, s, true);
    juce::Graphics g(square);
    g.drawImage(img.getClippedImage({minX, minY, cw, ch}).createCopy(),
                juce::Rectangle<float>((float) (s - cw) / 2.0f, (float) (s - ch) / 2.0f,
                                       (float) cw, (float) ch));
    return square;
}

class MainWindow : public juce::DocumentWindow {
public:
    MainWindow()
        : juce::DocumentWindow("Veyra Sounds",
                               juce::Colours::transparentBlack, // let the acrylic show
                               juce::DocumentWindow::allButtons)
    {
        setIcon(taskbarIcon()); // larger, padding-trimmed taskbar mark
        // Borderless: our TopBar is the custom glass title bar.
        setUsingNativeTitleBar(false);
        setTitleBarHeight(0);
        setBackgroundColour(juce::Colours::transparentBlack);
        setContentOwned(new veyra::ui::RootComponent(), true);
        // Fixed canvas (1600x900), like Spotify / SteelSeries Sonar: the layout is
        // designed around one size, so no resize and no maximise.
        setResizable(false, false);
        centreWithSize(1200, 675);
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
