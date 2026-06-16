#include "Components/EqBandSlider.h"

#include "Theme/Fonts.h"

namespace veyra::ui {

juce::Rectangle<int> EqBandSlider::railArea() const
{
    auto r = getLocalBounds();
    r.removeFromTop(6);     // headroom
    r.removeFromBottom(18); // freq label
    return r.reduced(0, 6);
}

void EqBandSlider::paint(juce::Graphics& g)
{
    const auto rail = railArea();
    const float railW = 6.0f;
    const float x = (float) rail.getCentreX() - railW * 0.5f;
    const float y = (float) rail.getY();
    const float h = (float) rail.getHeight();
    const float midY = y + h * 0.5f;

    g.setColour(juce::Colour(60, 62, 80).withAlpha(0.5f));
    g.fillRoundedRectangle(x, y, railW, h, railW * 0.5f);

    // Center-origin fill toward the thumb (thinner so the curve dominates).
    const float pct = gain_ / 12.0f; // -1..1
    const float thumbY = midY - pct * (h * 0.5f);
    g.setColour(palette_.accentPrimary.withAlpha(0.85f));
    if (gain_ >= 0)
        g.fillRoundedRectangle(x, thumbY, railW, midY - thumbY, railW * 0.5f);
    else
        g.fillRoundedRectangle(x, midY, railW, thumbY - midY, railW * 0.5f);

    // Center detent tick.
    g.setColour(palette_.strokeHover);
    g.drawLine(x - 3.0f, midY, x + railW + 3.0f, midY, 1.0f);

    // Thumb — grows + glows brighter on hover/drag for clear feedback.
    const bool active = hover_ || dragging_;
    const float tr = active ? 9.0f : 7.0f;
    const float cx = (float) rail.getCentreX();
    juce::DropShadow(palette_.accentGlow, active ? 14 : 8, {}).drawForRectangle(
        g, juce::Rectangle<float>(cx - tr, thumbY - tr, tr * 2, tr * 2).toNearestInt());
    g.setColour(juce::Colours::white);
    g.fillEllipse(cx - tr, thumbY - tr, tr * 2.0f, tr * 2.0f);
    g.setColour(palette_.accentPrimary);
    g.drawEllipse(cx - tr, thumbY - tr, tr * 2.0f, tr * 2.0f, active ? 2.4f : 1.8f);

    // While dragging, a compact freq + gain tooltip above the thumb; otherwise
    // just the dB value.
    if (dragging_)
    {
        const juce::String txt = freq_ + " Hz   " + (gain_ >= 0 ? "+" : "") + juce::String(gain_, 1) + " dB";
        const float tw = 92.0f, th = 16.0f;
        juce::Rectangle<float> bub((float) getWidth() * 0.5f - tw * 0.5f, thumbY - tr - th - 4.0f, tw, th);
        bub = bub.constrainedWithin(getLocalBounds().toFloat());
        g.setColour(palette_.bgGlassElevated);
        g.fillRoundedRectangle(bub, 5.0f);
        g.setColour(palette_.strokeActive);
        g.drawRoundedRectangle(bub.reduced(0.5f), 5.0f, 1.0f);
        g.setColour(palette_.textPrimary);
        g.setFont(fonts::mono(10.0f, true));
        g.drawText(txt, bub, juce::Justification::centred, false);
    }
    else
    {
        g.setColour(palette_.textPrimary);
        g.setFont(fonts::mono(10.0f));
        g.drawText((gain_ >= 0 ? "+" : "") + juce::String(gain_, 1),
                   juce::Rectangle<float>(0.0f, thumbY - tr - 15.0f, (float) getWidth(), 13.0f),
                   juce::Justification::centred, false);
    }

    // Frequency label (brighter when active).
    g.setColour(active ? palette_.textPrimary : palette_.textSecondary);
    g.setFont(fonts::mono(11.0f, active));
    g.drawText(freq_, getLocalBounds().removeFromBottom(16), juce::Justification::centred, false);
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
    return {(float) railArea().getCentreX(), yForGain(gain_)};
}

float EqBandSlider::yForGain(float db) const
{
    const auto rail = railArea();
    const float h = (float) rail.getHeight();
    const float midY = (float) rail.getY() + h * 0.5f;
    return midY - (juce::jlimit(-12.0f, 12.0f, db) / 12.0f) * (h * 0.5f);
}

void EqBandSlider::mouseDown(const juce::MouseEvent& e) { dragging_ = true; setFromMouseY((float) e.y); }
void EqBandSlider::mouseDrag(const juce::MouseEvent& e) { setFromMouseY((float) e.y); }
void EqBandSlider::mouseUp(const juce::MouseEvent&) { dragging_ = false; repaint(); }
void EqBandSlider::mouseEnter(const juce::MouseEvent&) { hover_ = true; repaint(); }
void EqBandSlider::mouseExit(const juce::MouseEvent&) { hover_ = false; repaint(); }

} // namespace veyra::ui
