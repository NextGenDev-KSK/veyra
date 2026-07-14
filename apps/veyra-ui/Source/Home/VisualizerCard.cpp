#include "Home/VisualizerCard.h"

#include "Theme/Fonts.h"

namespace veyra::ui {

VisualizerCard::VisualizerCard()
{
    mode_.addItemList({"Bars", "Monstercat", "Circular", "Ferrofluid", "Waveform",
                       "Particle", "Wavy", "3D Bars"}, 1);
    mode_.setSelectedId(1, juce::dontSendNotification);
    mode_.onChange = [this] { modeIndex_ = mode_.getSelectedId() - 1; repaint(); };
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

void VisualizerCard::setLiveFrame(const float* bars, int n,
                                  float vuL, float vuR, float peakL, float peakR, bool clip)
{
    const int m = juce::jmin(n, kBars);
    for (int i = 0; i < m; ++i)
        targets_[(size_t) i] = juce::jlimit(0.0f, 1.0f, bars[i]);
    vuL_ = juce::jlimit(0.0f, 1.0f, vuL);
    vuR_ = juce::jlimit(0.0f, 1.0f, vuR);
    peakL_ = juce::jmax(peakL_, juce::jlimit(0.0f, 1.0f, peakL));
    peakR_ = juce::jmax(peakR_, juce::jlimit(0.0f, 1.0f, peakR));
    clip_ = clip;
    liveTtl_ = 30; // ~0.5 s of live priority before idle fallback (at 60 Hz)
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
    if (liveTtl_ > 0)
    {
        // Live audio: targets_/VU set by setLiveFrame(); just ease + decay peaks.
        --liveTtl_;
    }
    else
    {
        // Idle animation when no service feed. Regenerate targets ~every 9th
        // frame, then ease toward them (calm, ~40% slower than per-frame).
        if (++frame_ % 9 == 0)
            for (int i = 0; i < kBars; ++i)
            {
                const float shape = 0.35f + 0.6f * std::sin((float) i / kBars * juce::MathConstants<float>::pi);
                targets_[(size_t) i] = std::pow(rng_.nextFloat(), 2.0f) * shape;
            }
        // Cap the random walk below the clip threshold so CLIP never trips on
        // idle data.
        vuL_ = juce::jlimit(0.0f, 0.85f, vuL_ + (rng_.nextFloat() - 0.5f) * 0.08f);
        vuR_ = juce::jlimit(0.0f, 0.85f, vuR_ + (rng_.nextFloat() - 0.5f) * 0.08f);
        clip_ = false;
    }

    for (int i = 0; i < kBars; ++i)
    {
        bars_[(size_t) i] += (targets_[(size_t) i] - bars_[(size_t) i]) * 0.10f;
        caps_[(size_t) i] = juce::jmax(bars_[(size_t) i], caps_[(size_t) i] - 0.018f); // Monstercat gravity
    }
    peakL_ = juce::jmax(vuL_, peakL_ - 0.01f); // peak-hold with slow decay
    peakR_ = juce::jmax(vuR_, peakR_ - 0.01f);
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
        // Audio meter scale: the dedicated meter tokens, never chrome colours.
        g.setColour(palette_.meterTrack);
        g.fillRoundedRectangle(x, my, mw, mh, 4.0f);

        juce::ColourGradient grad(palette_.meterLow, 0, my + mh, palette_.meterHigh, 0, my, false);
        grad.addColour(0.4, palette_.meterLow);
        grad.addColour(0.8, palette_.meterMid);
        g.setGradientFill(grad);
        const float fh = mh * level;
        g.fillRoundedRectangle(x, my + mh - fh, mw, fh, 4.0f);

        const float py = my + mh * (1.0f - peak); // peak-hold tick
        g.setColour(palette_.textPrimary);
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
        g.setColour(palette_.meterHigh);
        g.fillRoundedRectangle(clipBox, 4.0f);
        g.setColour(palette_.textOnAccent);
        g.setFont(fonts::mono(9.0f, true));
        g.drawText("CLIP", clipBox, juce::Justification::centred, false);
    }
    else
    {
        g.setColour(palette_.textTertiary);
        g.setFont(fonts::mono(9.0f, true));
        g.drawText("- DB", clipBox, juce::Justification::centredLeft, false);
    }

    // ---- Spectrum (mode-dependent render) ----
    drawSpectrum(g, area.toFloat());
}

void VisualizerCard::drawSpectrum(juce::Graphics& g, juce::Rectangle<float> a)
{
    const float baseY = a.getBottom();
    const float specH = a.getHeight();
    const auto accent = palette_.accentPrimary;

    auto bandX = [&](int i, float bw, float gap) { return a.getX() + i * (bw + gap); };
    float energy = 0.0f;
    for (int i = 0; i < kBars; ++i) energy += bars_[(size_t) i];
    energy /= kBars;

    switch (modeIndex_)
    {
    case 1: // Monstercat — bars with falling caps
    {
        const float gap = 3.0f, bw = (a.getWidth() - gap * (kBars - 1)) / kBars;
        for (int i = 0; i < kBars; ++i)
        {
            const float h = juce::jmax(2.0f, bars_[(size_t) i] * specH);
            const float x = bandX(i, bw, gap);
            g.setColour(accent);
            g.fillRoundedRectangle(x, baseY - h, bw, h, 2.0f);
            const float capY = baseY - juce::jmax(h, caps_[(size_t) i] * specH);
            g.setColour(palette_.accentSecondary);
            g.fillRect(x, capY - 2.0f, bw, 2.0f);
        }
        break;
    }
    case 2: // Circular — radial bars
    {
        const auto c = a.getCentre();
        const float rIn = juce::jmin(a.getWidth(), a.getHeight()) * 0.16f;
        const float rMax = juce::jmin(a.getWidth(), a.getHeight()) * 0.48f;
        for (int i = 0; i < kBars; ++i)
        {
            const float ang = (float) i / kBars * juce::MathConstants<float>::twoPi;
            const float len = rIn + bars_[(size_t) i] * (rMax - rIn);
            const float ca = std::cos(ang), sa = std::sin(ang);
            g.setColour(accent.withAlpha(0.5f + 0.5f * bars_[(size_t) i]));
            g.drawLine(c.x + ca * rIn, c.y + sa * rIn, c.x + ca * len, c.y + sa * len, 2.5f);
        }
        break;
    }
    case 3: // Ferrofluid — filled smooth envelope + glow
    {
        juce::Path p;
        p.startNewSubPath(a.getX(), baseY);
        for (int i = 0; i < kBars; ++i)
            p.lineTo(a.getX() + (float) i / (kBars - 1) * a.getWidth(),
                     baseY - juce::jmax(2.0f, bars_[(size_t) i] * specH));
        p.lineTo(a.getRight(), baseY);
        p.closeSubPath();
        juce::ColourGradient grad(accent.withAlpha(0.85f), 0, baseY - specH,
                                  accent.withAlpha(0.10f), 0, baseY, false);
        g.setGradientFill(grad);
        g.fillPath(p);
        break;
    }
    case 4: // Waveform — symmetric oscilloscope band
    {
        const float mid = a.getCentreY();
        juce::Path top, bot;
        top.startNewSubPath(a.getX(), mid);
        bot.startNewSubPath(a.getX(), mid);
        for (int i = 0; i < kBars; ++i)
        {
            const float x = a.getX() + (float) i / (kBars - 1) * a.getWidth();
            const float dev = bars_[(size_t) i] * specH * 0.45f;
            top.lineTo(x, mid - dev);
            bot.lineTo(x, mid + dev);
        }
        g.setColour(accent);
        g.strokePath(top, juce::PathStrokeType(2.0f));
        g.setColour(accent.withAlpha(0.5f));
        g.strokePath(bot, juce::PathStrokeType(2.0f));
        break;
    }
    case 5: // Particle — dots driven by band energy
    {
        for (int i = 0; i < kBars; ++i)
        {
            const float x = a.getX() + (float) i / (kBars - 1) * a.getWidth();
            const float y = baseY - bars_[(size_t) i] * specH;
            g.setColour(accent.withAlpha(0.85f));
            g.fillEllipse(x - 2.0f, y - 2.0f, 4.0f, 4.0f);
            g.setColour(palette_.accentSecondary.withAlpha(0.4f));
            g.fillEllipse(x - 1.0f, baseY - bars_[(size_t) i] * specH * 0.5f - 1.0f, 2.0f, 2.0f);
        }
        break;
    }
    case 6: // Wavy — stacked sine layers modulated by energy
    {
        const float mid = a.getCentreY();
        for (int layer = 0; layer < 3; ++layer)
        {
            juce::Path p;
            const float amp = specH * 0.18f * (1.0f + layer) * (0.3f + energy);
            const float phase = (float) frame_ * 0.06f + layer * 1.7f;
            for (int x = 0; x <= (int) a.getWidth(); x += 4)
            {
                const float fx = a.getX() + x;
                const float fy = mid + std::sin(x * 0.03f + phase) * amp;
                if (x == 0) p.startNewSubPath(fx, fy); else p.lineTo(fx, fy);
            }
            g.setColour(accent.withAlpha(0.25f + 0.2f * layer));
            g.strokePath(p, juce::PathStrokeType(2.0f));
        }
        break;
    }
    case 7: // 3D Bars — front face + lighter top face (faux perspective)
    {
        const float gap = 5.0f, bw = (a.getWidth() - gap * (kBars - 1)) / kBars;
        const float depth = 7.0f;
        for (int i = 0; i < kBars; ++i)
        {
            const float h = juce::jmax(2.0f, bars_[(size_t) i] * specH * 0.85f);
            const float x = bandX(i, bw, gap);
            const float topY = baseY - h;
            g.setColour(accent);
            g.fillRect(x, topY, bw, h);
            juce::Path top;
            top.startNewSubPath(x, topY);
            top.lineTo(x + depth, topY - depth);
            top.lineTo(x + bw + depth, topY - depth);
            top.lineTo(x + bw, topY);
            top.closeSubPath();
            g.setColour(accent.brighter(0.4f));
            g.fillPath(top);
        }
        break;
    }
    default: // Bars
    {
        const float gap = 3.0f, bw = (a.getWidth() - gap * (kBars - 1)) / kBars;
        for (int i = 0; i < kBars; ++i)
        {
            const float h = juce::jmax(2.0f, bars_[(size_t) i] * specH);
            const float x = bandX(i, bw, gap);
            juce::ColourGradient grad(accent, 0, baseY - h, accent.withAlpha(0.30f), 0, baseY, false);
            g.setGradientFill(grad);
            g.fillRoundedRectangle(x, baseY - h, bw, h, 2.0f);
        }
        break;
    }
    }
}

} // namespace veyra::ui
