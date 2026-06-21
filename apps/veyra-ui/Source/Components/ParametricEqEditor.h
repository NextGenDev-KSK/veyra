#pragma once

// Interactive parametric EQ editor: draggable nodes on a log-frequency response
// curve. Drag a node to set frequency (x) + gain (y); mouse-wheel sets Q;
// double-click empty space adds a bell; right-click removes a node. Emits the
// band list as veyra::ParametricBand for the config. The curve is the summed
// biquad magnitude response (computed with the shared RBJ designers).

#include "Theme/DesignTokens.h"
#include "VeyraGui.h"

#include "veyra/Config.h"

#include <functional>
#include <vector>

namespace veyra::ui {

class ParametricEqEditor : public juce::Component {
public:
    ParametricEqEditor();

    void setPalette(const Palette& p) { palette_ = p; repaint(); }
    void setBands(std::vector<veyra::ParametricBand> bands);
    const std::vector<veyra::ParametricBand>& bands() const { return bands_; }

    std::function<void(const std::vector<veyra::ParametricBand>&)> onChanged;

    void paint(juce::Graphics& g) override;
    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent& e) override;
    void mouseMove(const juce::MouseEvent& e) override;
    void mouseExit(const juce::MouseEvent& e) override;
    void mouseDoubleClick(const juce::MouseEvent& e) override;
    void mouseWheelMove(const juce::MouseEvent& e, const juce::MouseWheelDetails& w) override;

private:
    static constexpr float kFMin = 20.0f, kFMax = 20000.0f, kGMax = 12.0f;

    float freqToX(float f) const;
    float xToFreq(float x) const;
    float gainToY(float g) const;
    float yToGain(float y) const;
    int   hitTest(juce::Point<float> p) const; // band index or -1
    float curveDbAt(float freq) const;
    void  emit();

    void showBandMenu(int index);

    Palette palette_ = paletteForTheme("midnight");
    std::vector<veyra::ParametricBand> bands_;
    int dragging_ = -1;
    int selected_ = -1;
    int hovered_  = -1;
};

} // namespace veyra::ui
