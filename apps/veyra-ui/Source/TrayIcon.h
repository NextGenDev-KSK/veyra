#pragma once

// System tray icon — Phase 4c. A notification-area icon (the Veyra "V" mark
// rendered to an image) with a popup menu: open, mini mode, toggle master, quit.
// The shell wires the callbacks and refreshes the icon on theme change.

#include "Theme/DesignTokens.h"
#include "Theme/Fonts.h"
#include "VeyraGui.h"

#include <functional>

namespace veyra::ui {

class TrayIcon : public juce::SystemTrayIconComponent {
public:
    TrayIcon() { updateIcon(paletteForTheme("midnight")); }

    void updateIcon(const Palette& p)
    {
        juce::Image img(juce::Image::ARGB, 32, 32, true);
        juce::Graphics g(img);
        juce::Rectangle<float> r(2.0f, 2.0f, 28.0f, 28.0f);
        juce::ColourGradient grad(p.accentPrimary, r.getX(), r.getY(),
                                  p.accentPrimaryActive, r.getRight(), r.getBottom(), false);
        g.setGradientFill(grad);
        g.fillRoundedRectangle(r, 7.0f);
        g.setColour(juce::Colours::white);
        g.setFont(fonts::display(18.0f));
        g.drawText("V", r, juce::Justification::centred, false);

        setIconImage(img, img);
        setIconTooltip("Veyra Sounds");
    }

    std::function<void()>     onOpen, onMini, onQuit;
    std::function<void(bool)> onToggleMaster;
    std::function<bool()>     isMasterEnabled;

    void mouseDown(const juce::MouseEvent&) override { showMenu(); }

private:
    void showMenu()
    {
        const bool on = isMasterEnabled ? isMasterEnabled() : true;

        juce::PopupMenu m;
        m.addItem(1, "Open Veyra Sounds");
        m.addItem(2, "Mini Mode");
        m.addSeparator();
        m.addItem(3, "Master Enabled", true, on);
        m.addSeparator();
        m.addItem(4, "Quit Veyra");

        m.showMenuAsync(juce::PopupMenu::Options(), [this, on](int result)
        {
            switch (result)
            {
            case 1: if (onOpen) onOpen(); break;
            case 2: if (onMini) onMini(); break;
            case 3: if (onToggleMaster) onToggleMaster(!on); break;
            case 4: if (onQuit) onQuit(); break;
            default: break;
            }
        });
    }
};

} // namespace veyra::ui
