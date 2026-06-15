#include "Components/EqBandSlider.h"

#include "Theme/Fonts.h"

namespace veyra::ui {

juce::Rectangle<int> EqBandSlider::railArea() const
{
    auto r = getLocalBounds();
    r.removeFromTop(18);    // dB readout
    r.removeFromBottom(18); // freq label
    return r.reduced(0, 6);
}

void EqBandSlider::paint(juce::Graphics& g)
{
    auto top = getLocalBounds().removeFromTop(18);
    auto bottom = getLocalBounds().removeFromBottom(18);
    const auto rail = railArea();

    // dB readout.
    g.setColour(palette_.textSecondary);
    g.setFont(fonts::mono(11.0f));
    g.drawText((gain_ >= 0 ? "+" : "") + juce::String(gain_, 1) + " dB", top,
               juce::Justification::centred, false);

    // Frequency label.
    g.setColour(palette_.textTertiary);
    g.drawText(freq_, bottom, juce::Justification::centred, false);

    const float railW = 6.0f;
    const float x = (float) rail.getCentreX() - railW * 0.5f;
    const float y = (float) rail.getY();
    const float h = (float) rail.getHeight();
    const float midY = y + h * 0.5f;

    g.setColour(juce::Colour(60, 62, 80).withAlpha(0.5f));
    g.fillRoundedRectangle(x, y, railW, h, railW * 0.5f);

    // Center-origin fill toward the thumb.
    const float pct = gain_ / 12.0f; // -1..1
    const float thumbY = midY - pct * (h * 0.5f);
    g.setColour(palette_.accentPrimary);
    if (gain_ >= 0)
        g.fillRoundedRectangle(x, thumbY, railW, midY - thumbY, railW * 0.5f);
    else
        g.fillRoundedRectangle(x, midY, railW, thumbY - midY, railW * 0.5f);

    // Center detent tick.
    g.setColour(palette_.strokeHover);
    g.drawLine(x - 3.0f, midY, x + railW + 3.0f, midY, 1.0f);

    // Thumb with accent glow.
    const float tr = 11.0f;
    const float cx = (float) rail.getCentreX();
    juce::DropShadow(palette_.accentGlow, 12, {}).drawForRectangle(
        g, juce::Rectangle<float>(cx - tr, thumbY - tr, tr * 2, tr * 2).toNearestInt());
    g.setColour(juce::Colours::white);
    g.fillEllipse(cx - tr, thumbY - tr, tr * 2.0f, tr * 2.0f);
    g.setColour(palette_.accentPrimary);
    g.drawEllipse(cx - tr, thumbY - tr, tr * 2.0f, tr * 2.0f, 2.0f);
}

void EqBandSlider::setFromMouseY(float y)
{
    const auto rail = railArea();
    const float h = (float) rail.getHeight();
    const float midY = (float) rail.getY() + h * 0.5f;
    float g = ((midY - y) / (h * 0.5f)) * 12.0f;
    if (std::abs(g) < 0.3f)
        g = 0.0f; // snap-to-zero detent
    gain_ = juce::jlimit(-12.0f, 12.0f, g);
    if (onChange)
        onChange(gain_);
    repaint();
}

juce::Point<float> EqBandSlider::thumbCentre() const
{
    const auto rail = railArea();
    const float h = (float) rail.getHeight();
    const float midY = (float) rail.getY() + h * 0.5f;
    const float thumbY = midY - (gain_ / 12.0f) * (h * 0.5f);
    return {(float) rail.getCentreX(), thumbY};
}

void EqBandSlider::mouseDown(const juce::MouseEvent& e) { setFromMouseY((float) e.y); }
void EqBandSlider::mouseDrag(const juce::MouseEvent& e) { setFromMouseY((float) e.y); }

} // namespace veyra::ui
