#include "Home/VisualizerCard.h"

#include "Theme/Fonts.h"

namespace veyra::ui {

VisualizerCard::VisualizerCard()
{
    mode_.addItemList({"Bars", "Monstercat", "Circular", "Ferrofluid", "Waveform",
                       "Particle", "Wavy", "3D Bars"}, 1);
    mode_.setSelectedId(1, juce::dontSendNotification);
    addAndMakeVisible(mode_);
    addAndMakeVisible(fullscreen_);
    bars_.fill(8.0f);
    startTimerHz(60);
}

void VisualizerCard::setPalette(const Palette& p)
{
    GlassPanel::setPalette(p);
    fullscreen_.setPalette(p);
}

void VisualizerCard::resized()
{
    auto r = getLocalBounds().reduced(16);
    fullscreen_.setBounds(r.removeFromRight(32).removeFromTop(32));
    r.removeFromRight(8);
    mode_.setBounds(r.removeFromRight(110).removeFromTop(32));
}

void VisualizerCard::timerCallback()
{
    for (auto& b : bars_)
    {
        const float target = 6.0f + std::pow(rng_.nextFloat(), 2.0f) * 1.0f * (float) getHeight();
        b = juce::jmax(target, b * 0.92f);
    }
    vuL_ = juce::jlimit(0.0f, 1.0f, vuL_ + (rng_.nextFloat() - 0.5f) * 0.12f);
    vuR_ = juce::jlimit(0.0f, 1.0f, vuR_ + (rng_.nextFloat() - 0.5f) * 0.12f);
    repaint();
}

void VisualizerCard::paintContent(juce::Graphics& g)
{
    auto area = getLocalBounds().reduced(16);

    // Spectrum bars across the bottom.
    auto specArea = area.withTrimmedTop(48);
    const float gap = 3.0f;
    const float bw = ((float) specArea.getWidth() - gap * (kBars - 1)) / kBars;
    const float baseY = (float) specArea.getBottom();
    for (int i = 0; i < kBars; ++i)
    {
        const float hgt = juce::jmin(bars_[(size_t) i], (float) specArea.getHeight());
        const float x = (float) specArea.getX() + i * (bw + gap);
        juce::ColourGradient grad(palette_.accentPrimary, 0, baseY - hgt,
                                  palette_.accentPrimary.withAlpha(0.35f), 0, baseY, false);
        g.setGradientFill(grad);
        g.fillRoundedRectangle(x, baseY - hgt, bw, hgt, 2.0f);
    }

    // L/R VU meters (top-left).
    auto drawMeter = [&](float x, float level, const char* cap)
    {
        const float mh = 56.0f, my = (float) area.getY() + 4.0f, mw = 8.0f;
        g.setColour(juce::Colour(60, 62, 80).withAlpha(0.4f));
        g.fillRoundedRectangle(x, my, mw, mh, 4.0f);
        const float fh = mh * level;
        juce::ColourGradient grad(palette_.accentSecondary, 0, my + mh, palette_.danger, 0, my, false);
        grad.addColour(0.4, palette_.accentSecondary);
        grad.addColour(0.8, palette_.warning);
        g.setGradientFill(grad);
        g.fillRoundedRectangle(x, my + mh - fh, mw, fh, 4.0f);
        g.setColour(palette_.textTertiary);
        g.setFont(fonts::mono(9.0f));
        g.drawText(cap, juce::Rectangle<float>(x - 4.0f, my + mh + 2.0f, mw + 8.0f, 12.0f),
                   juce::Justification::centred, false);
    };
    drawMeter((float) area.getX(), vuL_, "L");
    drawMeter((float) area.getX() + 16.0f, vuR_, "R");

    // FPS badge.
    g.setColour(palette_.bgApp.withAlpha(0.4f));
    auto fps = juce::Rectangle<float>((float) area.getX(), (float) specArea.getY() - 4.0f, 64.0f, 20.0f);
    g.fillRoundedRectangle(fps, 6.0f);
    g.setColour(palette_.textSecondary);
    g.setFont(fonts::mono(11.0f));
    g.drawText("144 FPS", fps, juce::Justification::centred, false);
}

} // namespace veyra::ui
