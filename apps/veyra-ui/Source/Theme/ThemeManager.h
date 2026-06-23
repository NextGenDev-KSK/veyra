#pragma once

// Holds the active theme palette and notifies listeners on change. Theme
// switching is a token swap on the same component tree; the 400 ms cross-fade is
// applied by the view layer (RootComponent) when it repaints on change.

#include "Theme/DesignTokens.h"
#include "VeyraGui.h"

namespace veyra::ui {

class ThemeManager : public juce::ChangeBroadcaster {
public:
    ThemeManager() : palette_(paletteForTheme(currentId_)) {}

    void setTheme(const juce::String& id)
    {
        if (id == currentId_)
            return;
        currentId_ = id;
        rebuild();
        sendChangeMessage();
    }

    // Custom-theme accent (applied only while the Custom theme is active).
    void setCustomAccent(juce::Colour c)
    {
        customAccent_ = c;
        if (currentId_ == "custom")
        {
            rebuild();
            sendChangeMessage();
        }
    }

    const juce::String& currentId() const noexcept { return currentId_; }
    const Palette& palette() const noexcept { return palette_; }

private:
    void rebuild()
    {
        palette_ = (currentId_ == "custom") ? customPalette(customAccent_)
                                            : paletteForTheme(currentId_);
    }

    juce::String currentId_{"midnight"};
    juce::Colour customAccent_{0xff5b3fe4}; // default accent until the user picks one
    Palette palette_;
};

} // namespace veyra::ui
