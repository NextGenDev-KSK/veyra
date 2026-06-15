#include "Home/EqualizerCard.h"

#include "Theme/Fonts.h"

namespace veyra::ui {

namespace {
const char* kFreqLabels[] = {"32", "64", "125", "250", "500", "1k", "2k", "4k", "8k", "16k"};
const float kInitialGains[] = {1, -2, 0, 3, -1, -4, 1, 5, 6, -1};
} // namespace

EqualizerCard::EqualizerCard()
{
    for (int i = 0; i < kBands; ++i)
    {
        auto band = std::make_unique<EqBandSlider>(kFreqLabels[i]);
        band->setGainDb(kInitialGains[i]);
        band->onChange = [this, i](float db) { if (onBandChanged) onBandChanged(i, db); repaint(); };
        addAndMakeVisible(*band);
        bands_[(size_t) i] = std::move(band);
    }

    modeToggle_.setItems({"Graphic", "Parametric"});
    addAndMakeVisible(modeToggle_);

    showCurve_.setToggleState(true, juce::dontSendNotification);
    showCurve_.onClick = [this] { repaint(); };
    addAndMakeVisible(showCurve_);
    addAndMakeVisible(showSpectrum_);

    reset_.getProperties().set("variant", "ghost");
    reset_.onClick = [this]
    {
        for (auto& b : bands_) b->setGainDb(0.0f);
        repaint();
    };
    addAndMakeVisible(reset_);
}

void EqualizerCard::setPalette(const Palette& p)
{
    GlassPanel::setPalette(p);
    for (auto& b : bands_) b->setPalette(p);
    modeToggle_.setPalette(p);
    showCurve_.setPalette(p);
    showSpectrum_.setPalette(p);
}

void EqualizerCard::resized()
{
    auto content = getLocalBounds().reduced(24);
    auto header = content.removeFromTop(36);

    reset_.setBounds(header.removeFromRight(60).withSizeKeepingCentre(60, 28));
    header.removeFromRight(16);
    showSpectrum_.setBounds(header.removeFromRight(36).withSizeKeepingCentre(36, 20));
    header.removeFromRight(8 + 100); // gap + painted "Show spectrum" label
    showCurve_.setBounds(header.removeFromRight(36).withSizeKeepingCentre(36, 20));
    header.removeFromRight(8 + 78);  // gap + painted "Show curve" label
    modeToggle_.setBounds(header.removeFromRight(170).withSizeKeepingCentre(170, 32));

    content.removeFromTop(16);
    const int bw = content.getWidth() / kBands;
    for (int i = 0; i < kBands; ++i)
        bands_[(size_t) i]->setBounds(content.getX() + i * bw, content.getY(), bw, content.getHeight());
}

void EqualizerCard::paintContent(juce::Graphics& g)
{
    auto content = getLocalBounds().reduced(24);
    auto header = content.removeFromTop(36);

    g.setColour(palette_.textPrimary);
    g.setFont(fonts::display(20.0f));
    g.drawText("EQUALIZER", header, juce::Justification::centredLeft, false);

    // Toggle labels just left of each toggle.
    g.setColour(palette_.textSecondary);
    g.setFont(fonts::body(13.0f));
    g.drawText("Show curve",
               juce::Rectangle<int>(showCurve_.getX() - 80, header.getY(), 76, 36),
               juce::Justification::centredRight, false);
    g.drawText("Show spectrum",
               juce::Rectangle<int>(showSpectrum_.getX() - 102, header.getY(), 98, 36),
               juce::Justification::centredRight, false);

    // Response curve through the band thumbs (behind the sliders).
    if (showCurve_.getToggleState())
    {
        juce::Path curve;
        std::array<juce::Point<float>, kBands> pts;
        for (int i = 0; i < kBands; ++i)
            pts[(size_t) i] = bands_[(size_t) i]->getPosition().toFloat()
                              + bands_[(size_t) i]->thumbCentre();

        curve.startNewSubPath(pts[0]);
        for (int i = 1; i < kBands; ++i)
        {
            const auto a = pts[(size_t) i - 1];
            const auto b = pts[(size_t) i];
            const float midX = (a.x + b.x) * 0.5f;
            curve.cubicTo(midX, a.y, midX, b.y, b.x, b.y);
        }
        g.setColour(palette_.accentPrimary.withAlpha(0.55f));
        g.strokePath(curve, juce::PathStrokeType(2.0f));
    }
}

} // namespace veyra::ui
