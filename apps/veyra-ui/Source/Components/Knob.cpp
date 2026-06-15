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
    const float arcW = 10.0f;

    const bool off = dimWhenZero_ && value_ <= 0.0001;
    const float dim = off ? 0.35f : 1.0f;

    const float start = juce::MathConstants<float>::pi * 1.25f; // 225°
    const float end = juce::MathConstants<float>::pi * 2.75f;   // 495°
    const float angle = start + (end - start) * (float) value_;
    const float dangerAngle = start + (end - start) * juce::jmin(dangerThreshold_, 1.0f);

    auto strokeArc = [&](float a0, float a1, juce::Colour c, float w)
    {
        if (a1 <= a0) return;
        juce::Path p;
        p.addCentredArc(cx, cy, radius, radius, 0.0f, a0, a1, true);
        g.setColour(c);
        g.strokePath(p, juce::PathStrokeType(w, juce::PathStrokeType::curved,
                                             juce::PathStrokeType::rounded));
    };

    // Background track (+ faint danger zone marker if applicable).
    strokeArc(start, end, juce::Colour(60, 62, 80).withAlpha(0.4f), arcW);
    if (dangerThreshold_ <= 1.0f)
        strokeArc(dangerAngle, end, palette_.danger.withAlpha(0.20f), arcW);

    // Value arc: accent up to the danger threshold, warning beyond it.
    if (value_ > 0.0001)
    {
        const float safeEnd = juce::jmin(angle, dangerAngle);
        strokeArc(start, safeEnd, palette_.accentPrimary.withMultipliedAlpha(dim), arcW);
        if (angle > dangerAngle)
            strokeArc(dangerAngle, angle, palette_.warning, arcW);
    }

    // Inner disc with depth (radial light from top, dark base + bevel).
    const float ir = radius - 11.0f;
    juce::ColourGradient disc(palette_.bgGlassElevated.brighter(0.10f), cx, cy - ir,
                              palette_.bgApp, cx, cy + ir, false);
    g.setGradientFill(disc);
    g.fillEllipse(cx - ir, cy - ir, ir * 2.0f, ir * 2.0f);
    g.setColour(palette_.strokeLightLeak); // top light-leak bevel
    g.drawEllipse(cx - ir, cy - ir + 0.5f, ir * 2.0f, ir * 2.0f, 1.0f);
    g.setColour(juce::Colours::black.withAlpha(0.25f)); // bottom inner shadow
    g.drawEllipse(cx - ir, cy - ir - 0.5f, ir * 2.0f, ir * 2.0f, 1.0f);

    // Indicator line.
    const float sx = std::sin(angle);
    const float cyn = -std::cos(angle);
    g.setColour(juce::Colours::white.withMultipliedAlpha(dim));
    g.drawLine(cx + sx * ir * 0.35f, cy + cyn * ir * 0.35f, cx + sx * ir, cy + cyn * ir, 2.5f);

    // LED dot at 12 o'clock.
    juce::Colour led = off ? palette_.textTertiary
                       : (angle > dangerAngle ? palette_.warning : palette_.accentPrimary);
    g.setColour(led);
    g.fillEllipse(cx - 2.5f, knob.getY() + 1.0f, 5.0f, 5.0f);

    // Value text.
    g.setColour(off ? palette_.textTertiary : palette_.textPrimary);
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
