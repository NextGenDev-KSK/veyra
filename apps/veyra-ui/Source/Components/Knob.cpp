#include "Components/Knob.h"

#include "Theme/Fonts.h"

namespace veyra::ui {

void Knob::paint(juce::Graphics& g)
{
    auto area = getLocalBounds();
    auto labelArea = area.removeFromTop(16);
    auto valueArea = area.removeFromBottom(16);
    auto knob = area.toFloat();

    // Label.
    g.setColour(palette_.textSecondary);
    g.setFont(fonts::body(12.0f));
    g.drawText(label_, labelArea, juce::Justification::centred, false);

    const float radius = juce::jmin(knob.getWidth(), knob.getHeight()) * 0.5f - 6.0f;
    const float cx = knob.getCentreX();
    const float cy = knob.getCentreY();

    const float start = juce::MathConstants<float>::pi * 1.25f; // 225°
    const float end = juce::MathConstants<float>::pi * 2.75f;   // 495°
    const float angle = start + (end - start) * (float) value_;

    // Background arc.
    juce::Path bg;
    bg.addCentredArc(cx, cy, radius, radius, 0.0f, start, end, true);
    g.setColour(juce::Colour(60, 62, 80).withAlpha(0.4f));
    g.strokePath(bg, juce::PathStrokeType(8.0f, juce::PathStrokeType::curved,
                                          juce::PathStrokeType::rounded));

    // Value arc.
    if (value_ > 0.0001)
    {
        juce::Path val;
        val.addCentredArc(cx, cy, radius, radius, 0.0f, start, angle, true);
        g.setColour(palette_.accentPrimary);
        g.strokePath(val, juce::PathStrokeType(8.0f, juce::PathStrokeType::curved,
                                               juce::PathStrokeType::rounded));
    }

    // Inner glass disc.
    const float ir = radius - 9.0f;
    g.setColour(palette_.bgGlassElevated);
    g.fillEllipse(cx - ir, cy - ir, ir * 2.0f, ir * 2.0f);
    g.setColour(palette_.strokeDefault);
    g.drawEllipse(cx - ir, cy - ir, ir * 2.0f, ir * 2.0f, 1.0f);

    // Indicator line.
    const float sx = std::sin(angle);
    const float cyn = -std::cos(angle);
    g.setColour(juce::Colours::white);
    g.drawLine(cx + sx * ir * 0.35f, cy + cyn * ir * 0.35f,
               cx + sx * ir, cy + cyn * ir, 2.0f);

    // LED dot at 12 o'clock of the knob area.
    g.setColour(value_ > 0.0001 ? palette_.accentPrimary : palette_.textTertiary);
    g.fillEllipse(cx - 2.0f, knob.getY() + 2.0f, 4.0f, 4.0f);

    // Value text.
    g.setColour(palette_.textPrimary);
    g.setFont(fonts::mono(12.0f));
    g.drawText(valueText_, valueArea, juce::Justification::centred, false);
}

void Knob::mouseDown(const juce::MouseEvent&)
{
    dragStartValue_ = value_;
}

void Knob::mouseDrag(const juce::MouseEvent& e)
{
    const double delta = -e.getDistanceFromDragStartY() * 0.005;
    value_ = juce::jlimit(0.0, 1.0, dragStartValue_ + delta);
    if (onChange)
        onChange(value_);
    repaint();
}

} // namespace veyra::ui
