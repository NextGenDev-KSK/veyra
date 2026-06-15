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
        palette_ = paletteForTheme(id);
        sendChangeMessage();
    }

    const juce::String& currentId() const noexcept { return currentId_; }
    const Palette& palette() const noexcept { return palette_; }

private:
    juce::String currentId_{"midnight"};
    Palette palette_;
};

} // namespace veyra::ui
