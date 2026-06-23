#pragma once

// Minimal line-icon set drawn with JUCE Paths (Phosphor-style, stroked). Each
// draws inside the given box in the given colour. Good enough for Phase 4b;
// swappable for bundled Phosphor SVGs later.

#include "VeyraGui.h"

namespace veyra::ui::icons {

inline void stroke(juce::Graphics& g, const juce::Path& p, juce::Colour c, float w = 1.8f)
{
    g.setColour(c);
    g.strokePath(p, juce::PathStrokeType(w, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
}

// Map normalised (0..1) coordinates into the icon box.
struct Mapper {
    juce::Rectangle<float> r;
    float x(float v) const { return r.getX() + v * r.getWidth(); }
    float y(float v) const { return r.getY() + v * r.getHeight(); }
};

inline void home(juce::Graphics& g, juce::Rectangle<float> r, juce::Colour c)
{
    Mapper m{r}; juce::Path p;
    p.startNewSubPath(m.x(0.08f), m.y(0.5f));
    p.lineTo(m.x(0.5f), m.y(0.12f));
    p.lineTo(m.x(0.92f), m.y(0.5f));
    p.startNewSubPath(m.x(0.2f), m.y(0.45f));
    p.lineTo(m.x(0.2f), m.y(0.9f));
    p.lineTo(m.x(0.8f), m.y(0.9f));
    p.lineTo(m.x(0.8f), m.y(0.45f));
    stroke(g, p, c);
}
inline void presets(juce::Graphics& g, juce::Rectangle<float> r, juce::Colour c)
{
    Mapper m{r}; juce::Path p;
    p.startNewSubPath(m.x(0.25f), m.y(0.12f));
    p.lineTo(m.x(0.75f), m.y(0.12f));
    p.lineTo(m.x(0.75f), m.y(0.9f));
    p.lineTo(m.x(0.5f), m.y(0.68f));
    p.lineTo(m.x(0.25f), m.y(0.9f));
    p.closeSubPath();
    stroke(g, p, c);
}
inline void apps(juce::Graphics& g, juce::Rectangle<float> r, juce::Colour c)
{
    Mapper m{r};
    g.setColour(c);
    for (float gx : {0.18f, 0.62f})
        for (float gy : {0.18f, 0.62f})
            g.drawRoundedRectangle(m.x(gx), m.y(gy), r.getWidth() * 0.2f, r.getHeight() * 0.2f, 1.5f, 1.6f);
}
inline void devices(juce::Graphics& g, juce::Rectangle<float> r, juce::Colour c)
{
    Mapper m{r}; juce::Path p;
    p.startNewSubPath(m.x(0.12f), m.y(0.38f));
    p.lineTo(m.x(0.32f), m.y(0.38f));
    p.lineTo(m.x(0.5f), m.y(0.18f));
    p.lineTo(m.x(0.5f), m.y(0.82f));
    p.lineTo(m.x(0.32f), m.y(0.62f));
    p.lineTo(m.x(0.12f), m.y(0.62f));
    p.closeSubPath();
    p.startNewSubPath(m.x(0.68f), m.y(0.36f));
    p.quadraticTo(m.x(0.82f), m.y(0.5f), m.x(0.68f), m.y(0.64f));
    stroke(g, p, c);
}
inline void flask(juce::Graphics& g, juce::Rectangle<float> r, juce::Colour c)
{
    Mapper m{r}; juce::Path p;
    p.startNewSubPath(m.x(0.4f), m.y(0.12f));
    p.lineTo(m.x(0.4f), m.y(0.45f));
    p.lineTo(m.x(0.2f), m.y(0.85f));
    p.lineTo(m.x(0.8f), m.y(0.85f));
    p.lineTo(m.x(0.6f), m.y(0.45f));
    p.lineTo(m.x(0.6f), m.y(0.12f));
    p.startNewSubPath(m.x(0.33f), m.y(0.12f));
    p.lineTo(m.x(0.67f), m.y(0.12f));
    stroke(g, p, c);
}
inline void gamer(juce::Graphics& g, juce::Rectangle<float> r, juce::Colour c)
{
    Mapper m{r};
    g.setColour(c);
    g.drawRoundedRectangle(m.x(0.1f), m.y(0.32f), r.getWidth() * 0.8f, r.getHeight() * 0.38f,
                           r.getHeight() * 0.19f, 1.8f);
    juce::Path p;
    p.startNewSubPath(m.x(0.24f), m.y(0.51f)); p.lineTo(m.x(0.36f), m.y(0.51f));
    p.startNewSubPath(m.x(0.3f), m.y(0.45f)); p.lineTo(m.x(0.3f), m.y(0.57f));
    stroke(g, p, c);
    g.fillEllipse(m.x(0.62f), m.y(0.45f), r.getWidth() * 0.07f, r.getWidth() * 0.07f);
    g.fillEllipse(m.x(0.72f), m.y(0.55f), r.getWidth() * 0.07f, r.getWidth() * 0.07f);
}
inline void gear(juce::Graphics& g, juce::Rectangle<float> r, juce::Colour c)
{
    Mapper m{r};
    g.setColour(c);
    const float cx = m.x(0.5f), cy = m.y(0.5f);
    const float ro = r.getWidth() * 0.38f, ri = r.getWidth() * 0.16f;
    juce::Path p;
    for (int i = 0; i < 8; ++i)
    {
        const float a = juce::MathConstants<float>::twoPi * i / 8.0f;
        p.startNewSubPath(cx + std::cos(a) * ri, cy + std::sin(a) * ri);
        p.lineTo(cx + std::cos(a) * ro, cy + std::sin(a) * ro);
    }
    stroke(g, p, c, 1.6f);
    g.drawEllipse(cx - ri, cy - ri, ri * 2, ri * 2, 1.8f);
}
inline void search(juce::Graphics& g, juce::Rectangle<float> r, juce::Colour c)
{
    Mapper m{r};
    g.setColour(c);
    const float d = r.getWidth() * 0.5f;
    g.drawEllipse(m.x(0.12f), m.y(0.12f), d, d, 1.8f);
    juce::Path p; p.startNewSubPath(m.x(0.62f), m.y(0.62f)); p.lineTo(m.x(0.88f), m.y(0.88f));
    stroke(g, p, c);
}
inline void bell(juce::Graphics& g, juce::Rectangle<float> r, juce::Colour c)
{
    Mapper m{r}; juce::Path p;
    p.startNewSubPath(m.x(0.25f), m.y(0.7f));
    p.lineTo(m.x(0.25f), m.y(0.42f));
    p.quadraticTo(m.x(0.5f), m.y(0.1f), m.x(0.75f), m.y(0.42f));
    p.lineTo(m.x(0.75f), m.y(0.7f));
    p.lineTo(m.x(0.18f), m.y(0.7f));
    p.lineTo(m.x(0.82f), m.y(0.7f));
    stroke(g, p, c);
}
inline void fullscreen(juce::Graphics& g, juce::Rectangle<float> r, juce::Colour c)
{
    Mapper m{r}; juce::Path p;
    p.startNewSubPath(m.x(0.12f), m.y(0.35f)); p.lineTo(m.x(0.12f), m.y(0.12f)); p.lineTo(m.x(0.35f), m.y(0.12f));
    p.startNewSubPath(m.x(0.88f), m.y(0.35f)); p.lineTo(m.x(0.88f), m.y(0.12f)); p.lineTo(m.x(0.65f), m.y(0.12f));
    p.startNewSubPath(m.x(0.12f), m.y(0.65f)); p.lineTo(m.x(0.12f), m.y(0.88f)); p.lineTo(m.x(0.35f), m.y(0.88f));
    p.startNewSubPath(m.x(0.88f), m.y(0.65f)); p.lineTo(m.x(0.88f), m.y(0.88f)); p.lineTo(m.x(0.65f), m.y(0.88f));
    stroke(g, p, c);
}
inline void minimise(juce::Graphics& g, juce::Rectangle<float> r, juce::Colour c)
{
    Mapper m{r}; juce::Path p; p.startNewSubPath(m.x(0.2f), m.y(0.5f)); p.lineTo(m.x(0.8f), m.y(0.5f));
    stroke(g, p, c, 1.5f);
}
inline void maximise(juce::Graphics& g, juce::Rectangle<float> r, juce::Colour c)
{
    g.setColour(c);
    g.drawRoundedRectangle(r.getX() + r.getWidth() * 0.2f, r.getY() + r.getHeight() * 0.2f,
                           r.getWidth() * 0.6f, r.getHeight() * 0.6f, 2.0f, 1.5f);
}
inline void close(juce::Graphics& g, juce::Rectangle<float> r, juce::Colour c)
{
    Mapper m{r}; juce::Path p;
    p.startNewSubPath(m.x(0.2f), m.y(0.2f)); p.lineTo(m.x(0.8f), m.y(0.8f));
    p.startNewSubPath(m.x(0.8f), m.y(0.2f)); p.lineTo(m.x(0.2f), m.y(0.8f));
    stroke(g, p, c, 1.5f);
}
inline void plus(juce::Graphics& g, juce::Rectangle<float> r, juce::Colour c)
{
    Mapper m{r}; juce::Path p;
    p.startNewSubPath(m.x(0.5f), m.y(0.2f)); p.lineTo(m.x(0.5f), m.y(0.8f));
    p.startNewSubPath(m.x(0.2f), m.y(0.5f)); p.lineTo(m.x(0.8f), m.y(0.5f));
    stroke(g, p, c);
}
inline void chevronLeft(juce::Graphics& g, juce::Rectangle<float> r, juce::Colour c)
{
    Mapper m{r}; juce::Path p;
    p.startNewSubPath(m.x(0.62f), m.y(0.22f)); p.lineTo(m.x(0.36f), m.y(0.5f)); p.lineTo(m.x(0.62f), m.y(0.78f));
    stroke(g, p, c, 1.7f);
}
inline void chevronRight(juce::Graphics& g, juce::Rectangle<float> r, juce::Colour c)
{
    Mapper m{r}; juce::Path p;
    p.startNewSubPath(m.x(0.38f), m.y(0.22f)); p.lineTo(m.x(0.64f), m.y(0.5f)); p.lineTo(m.x(0.38f), m.y(0.78f));
    stroke(g, p, c, 1.7f);
}
inline void spatial(juce::Graphics& g, juce::Rectangle<float> r, juce::Colour c)
{
    g.setColour(c);
    auto box = r.reduced(r.getWidth() * 0.2f, r.getHeight() * 0.2f);
    g.drawEllipse(box, 1.6f); // sphere
    // orbit ring across it
    g.drawEllipse(box.getCentreX() - box.getWidth() * 0.5f,
                  box.getCentreY() - box.getHeight() * 0.2f,
                  box.getWidth(), box.getHeight() * 0.4f, 1.3f);
}
inline void dots(juce::Graphics& g, juce::Rectangle<float> r, juce::Colour c)
{
    g.setColour(c);
    const float cx = r.getCentreX(), d = 1.9f;
    for (float fy : {0.28f, 0.5f, 0.72f})
        g.fillEllipse(cx - d, r.getY() + r.getHeight() * fy - d, d * 2.0f, d * 2.0f);
}

} // namespace veyra::ui::icons
