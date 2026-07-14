#pragma once

// Holds the active theme palette and notifies listeners on change. A theme is
// data: switching swaps the palette object on the same component tree and the
// view layer repaints on the change message — live, no restart.

#include "Theme/DesignTokens.h"
#include "VeyraGui.h"

#include "veyra/ThemeTokens.h"

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

    // Custom-theme anchors (applied live while the Custom theme is active).
    void setCustomAnchors(const veyra::theme::CustomAnchors& anchors)
    {
        anchors_ = anchors;
        if (currentId_ == veyra::theme::kCustomThemeId)
        {
            rebuild();
            sendChangeMessage();
        }
    }

    const juce::String& currentId() const noexcept { return currentId_; }
    const Palette& palette() const noexcept { return palette_; }
    const veyra::theme::CustomAnchors& customAnchors() const noexcept { return anchors_; }

private:
    void rebuild()
    {
        palette_ = (currentId_ == veyra::theme::kCustomThemeId)
                       ? paletteFromTokens(veyra::theme::deriveCustom(anchors_))
                       : paletteForTheme(currentId_);
    }

    juce::String                currentId_{veyra::theme::kDefaultThemeId};
    veyra::theme::CustomAnchors anchors_;
    Palette                     palette_;
};

} // namespace veyra::ui
