#pragma once

// The app background behind all glass: a deep base fill, a few slow accent
// "ambient blobs", and a faint film-grain noise overlay. (The spec's blurred-
// desktop-capture mode is a later enhancement; this gives glass something rich
// to refract over.)

#include "Theme/DesignTokens.h"
#include "VeyraGui.h"

namespace veyra::ui {

class GlassBackground : public juce::Component {
public:
    GlassBackground() { setInterceptsMouseClicks(false, false); }

    void setPalette(const Palette& p) { palette_ = p; repaint(); }
    void paint(juce::Graphics&) override;

private:
    void ensureNoise();

    Palette palette_ = paletteForTheme("midnight");
    juce::Image noise_;
};

} // namespace veyra::ui
