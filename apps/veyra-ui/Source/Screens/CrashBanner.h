#pragma once

// Crash-recovery banner (spec §15): a dismissible strip shown at the top of the
// content when a previous session left a crash report in %APPDATA%\Veyra\crashes.
// Never sends anything — only offers to open the report folder.

#include "Theme/DesignTokens.h"
#include "VeyraGui.h"

#include <functional>

namespace veyra::ui {

class CrashBanner : public juce::Component {
public:
    CrashBanner();

    void setPalette(const Palette& p);
    void setMessage(juce::String text);
    void paint(juce::Graphics& g) override;
    void resized() override;

    std::function<void()> onOpenFolder;
    std::function<void()> onDismiss;

private:
    Palette          palette_ = paletteForTheme("midnight");
    juce::String     msg_;
    juce::TextButton open_{"Open Folder"}, dismiss_{"Dismiss"};
};

} // namespace veyra::ui
