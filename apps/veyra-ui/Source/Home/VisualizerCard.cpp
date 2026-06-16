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
    bars_.fill(0.2f);
    targets_.fill(0.2f);
    startTimerHz(60);
}

void VisualizerCard::setPalette(const Palette& p)
{
    GlassPanel::setPalette(p);
    fullscreen_.setPalette(p);
}

void VisualizerCard::setReduceMotion(bool reduce)
{
    if (reduce)
        stopTimer();
    else if (!isTimerRunning())
        startTimerHz(60);
    repaint();
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
    // Regenerate targets only ~every 9th frame, then ease toward them: this is
    // roughly 40% slower / calmer than per-frame target changes.
    if (++frame_ % 9 == 0)
        for (int i = 0; i < kBars; ++i)
        {
            const float shape = 0.35f + 0.6f * std::sin((float) i / kBars * juce::MathConstants<float>::pi);
            targets_[(size_t) i] = std::pow(rng_.nextFloat(), 2.0f) * shape;
        }
    for (int i = 0; i < kBars; ++i)
        bars_[(size_t) i] += (targets_[(size_t) i] - bars_[(size_t) i]) * 0.10f;

    vuL_ = juce::jlimit(0.0f, 1.0f, vuL_ + (rng_.nextFloat() - 0.5f) * 0.08f);
    vuR_ = juce::jlimit(0.0f, 1.0f, vuR_ + (rng_.nextFloat() - 0.5f) * 0.08f);
    peakL_ = juce::jmax(vuL_, peakL_ - 0.01f); // peak-hold with slow decay
    peakR_ = juce::jmax(vuR_, peakR_ - 0.01f);
    clip_ = (vuL_ > 0.96f || vuR_ > 0.96f);
    repaint();
}

void VisualizerCard::paintContent(juce::Graphics& g)
{
    auto area = getLocalBounds().reduced(16);

    // ---- VU column on the left (prominent monitoring) ----
    auto vuCol = area.removeFromLeft(70);
    area.removeFromLeft(14);

    const float mw = 12.0f, gap = 8.0f;
    const float capH = 16.0f, clipH = 16.0f;
    const float my = (float) vuCol.getY() + clipH;
    const float mh = (float) vuCol.getHeight() - clipH - capH;

    auto drawMeter = [&](float x, float level, float peak, const char* cap)
    {
        g.setColour(juce::Colour(60, 62, 80).withAlpha(0.4f));
        g.fillRoundedRectangle(x, my, mw, mh, 4.0f);

        juce::ColourGradient grad(palette_.accentSecondary, 0, my + mh, palette_.danger, 0, my, false);
        grad.addColour(0.4, palette_.accentSecondary);
        grad.addColour(0.8, palette_.warning);
        g.setGradientFill(grad);
        const float fh = mh * level;
        g.fillRoundedRectangle(x, my + mh - fh, mw, fh, 4.0f);

        const float py = my + mh * (1.0f - peak); // peak-hold tick
        g.setColour(juce::Colours::white);
        g.fillRect(x, py, mw, 2.0f);

        g.setColour(palette_.textTertiary);
        g.setFont(fonts::mono(10.0f));
        g.drawText(cap, juce::Rectangle<float>(x - 2.0f, my + mh + 2.0f, mw + 4.0f, 12.0f),
                   juce::Justification::centred, false);
    };
    const float vx = (float) vuCol.getX() + 8.0f;
    drawMeter(vx, vuL_, peakL_, "L");
    drawMeter(vx + mw + gap, vuR_, peakR_, "R");

    // CLIP indicator above the meters.
    auto clipBox = juce::Rectangle<float>((float) vuCol.getX(), (float) vuCol.getY(), 64.0f, 14.0f);
    if (clip_)
    {
        g.setColour(palette_.danger);
        g.fillRoundedRectangle(clipBox, 4.0f);
        g.setColour(juce::Colours::white);
        g.setFont(fonts::mono(9.0f, true));
        g.drawText("CLIP", clipBox, juce::Justification::centred, false);
    }
    else
    {
        g.setColour(palette_.textTertiary);
        g.setFont(fonts::mono(9.0f, true));
        g.drawText("- DB", clipBox, juce::Justification::centredLeft, false);
    }

    // ---- Spectrum (normalised heights) ----
    const float sgap = 3.0f;
    const float bw = ((float) area.getWidth() - sgap * (kBars - 1)) / kBars;
    const float baseY = (float) area.getBottom();
    const float specH = (float) area.getHeight();
    for (int i = 0; i < kBars; ++i)
    {
        const float hgt = juce::jmax(2.0f, bars_[(size_t) i] * specH);
        const float x = (float) area.getX() + i * (bw + sgap);
        juce::ColourGradient grad(palette_.accentPrimary, 0, baseY - hgt,
                                  palette_.accentPrimary.withAlpha(0.30f), 0, baseY, false);
        g.setGradientFill(grad);
        g.fillRoundedRectangle(x, baseY - hgt, bw, hgt, 2.0f);
    }
}

} // namespace veyra::ui
