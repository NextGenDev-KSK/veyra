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

    // Appearance settings (persisted). opacity is the alpha of the base fill:
    // lower = more of the window's acrylic backdrop (the blurred desktop) shows
    // through. It's applied at paint time, so dragging the slider is cheap (no
    // image rebuild — fixes the lag). mode: 0 ambient blobs, 1 solid, 2 image.
    void setOpacity(float o) { opacity_ = juce::jlimit(0.0f, 1.0f, o); repaint(); }
    void setBackgroundMode(int m) { bgMode_ = m; rebuild(); repaint(); }

    void paint(juce::Graphics&) override;
    void resized() override { rebuild(); }

    // Blurred low-res copy of the background, for glass surfaces to blit.
    const juce::Image& blurred() const noexcept { return blurred_; }

private:
    void rebuild();
    void renderSharp(juce::Image&) const;

    Palette palette_ = paletteForTheme("midnight");
    float opacity_ = 0.85f;
    int   bgMode_ = 0;
    juce::Image sharp_;
    juce::Image blurred_;
    static constexpr int kDownscale = 3;
};

} // namespace veyra::ui
