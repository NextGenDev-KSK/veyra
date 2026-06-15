#pragma once

// The app background behind all glass: a deep base fill, accent "ambient blobs",
// and a faint film-grain overlay — rendered to an image, plus a cached blurred,
// low-resolution copy that glass surfaces sample to fake backdrop-filter (JUCE
// has no native backdrop blur). This is what gives the glass its premium frost.

#include "Theme/DesignTokens.h"
#include "VeyraGui.h"

namespace veyra::ui {

class GlassBackground : public juce::Component {
public:
    GlassBackground() { setInterceptsMouseClicks(false, false); }

    void setPalette(const Palette& p) { palette_ = p; rebuild(); repaint(); }
    void paint(juce::Graphics&) override;
    void resized() override { rebuild(); }

    // Blurred low-res copy of the background, for glass surfaces to blit.
    const juce::Image& blurred() const noexcept { return blurred_; }

private:
    void rebuild();
    void renderSharp(juce::Image&) const;

    Palette palette_ = paletteForTheme("midnight");
    juce::Image sharp_;
    juce::Image blurred_;
    static constexpr int kDownscale = 3;
};

} // namespace veyra::ui
