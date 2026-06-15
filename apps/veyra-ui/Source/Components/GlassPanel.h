#pragma once

// Base for glass cards: holds a pointer to the shared backdrop + palette and
// paints the glass recipe. Subclasses override paintContent() to draw on top
// (or just add child components).

#include "Graphics/Glass.h"
#include "Theme/DesignTokens.h"
#include "VeyraGui.h"

namespace veyra::ui {

class GlassPanel : public juce::Component {
public:
    void setBackdrop(GlassBackground* b) { backdrop_ = b; repaint(); }
    virtual void setPalette(const Palette& p) { palette_ = p; repaint(); }
    void setElevated(bool e) { elevated_ = e; repaint(); }
    void setCornerRadius(float r) { radius_ = r; repaint(); }

    void paint(juce::Graphics& g) final
    {
        paintGlass(g, *this, backdrop_, palette_, radius_, elevated_);
        paintContent(g);
    }

protected:
    virtual void paintContent(juce::Graphics&) {}

    GlassBackground* backdrop_ = nullptr;
    Palette palette_ = paletteForTheme("midnight");
    bool elevated_ = false;
    float radius_ = 16.0f;
};

} // namespace veyra::ui
