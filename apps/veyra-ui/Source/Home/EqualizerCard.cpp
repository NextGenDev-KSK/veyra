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
    modeToggle_.onChange = [this](int i) { if (onModeChanged) onModeChanged(i == 1); repaint(); };
    addAndMakeVisible(modeToggle_);

    showCurve_.setToggleState(true, juce::dontSendNotification);
    showCurve_.onClick = [this] { repaint(); };
    addAndMakeVisible(showCurve_);
    addAndMakeVisible(showSpectrum_);

    reset_.getProperties().set("variant", "ghost");
    reset_.onClick = [this]
    {
        for (int i = 0; i < kBands; ++i)
        {
            bands_[(size_t) i]->setGainDb(0.0f);
            if (onBandChanged)
                onBandChanged(i, 0.0f);
        }
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

void EqualizerCard::setBandGain(int index, float db)
{
    if (index >= 0 && index < kBands)
    {
        bands_[(size_t) index]->setGainDb(db);
        repaint();
    }
}

float EqualizerCard::bandGain(int index) const
{
    return (index >= 0 && index < kBands) ? bands_[(size_t) index]->gainDb() : 0.0f;
}

void EqualizerCard::setMode(bool parametric)
{
    modeToggle_.setSelectedIndex(parametric ? 1 : 0, false);
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

    // dB grid behind the bands (0 / +/-6 / +/-12) for readability.
    if (! bands_[0])
        return;
    const int gridL = bands_[0]->getX();
    const int gridR = bands_[(size_t) kBands - 1]->getRight();
    const int band0Y = bands_[0]->getPosition().y;

    auto lineForDb = [&](float db) -> float { return (float) band0Y + bands_[0]->yForGain(db); };
    struct Grid { float db; const char* label; };
    for (const auto& gline : {Grid{12, "+12"}, Grid{6, "+6"}, Grid{0, "0"}, Grid{-6, "-6"}, Grid{-12, "-12"}})
    {
        const float y = lineForDb(gline.db);
        g.setColour(gline.db == 0 ? palette_.strokeHover : palette_.strokeDefault);
        g.drawLine((float) gridL, y, (float) gridR, y, 1.0f);
        g.setColour(palette_.textTertiary);
        g.setFont(fonts::mono(9.0f));
        g.drawText(gline.label, juce::Rectangle<float>((float) gridR + 2.0f, y - 7.0f, 28.0f, 14.0f),
                   juce::Justification::centredLeft, false);
    }

    // Dominant, glowing response curve through the band thumbs.
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
        // Wide translucent under-stroke (glow) + sharp bright over-stroke.
        g.setColour(palette_.accentGlow);
        g.strokePath(curve, juce::PathStrokeType(8.0f, juce::PathStrokeType::curved));
        g.setColour(palette_.accentPrimary);
        g.strokePath(curve, juce::PathStrokeType(3.5f, juce::PathStrokeType::curved));
    }
}

} // namespace veyra::ui
