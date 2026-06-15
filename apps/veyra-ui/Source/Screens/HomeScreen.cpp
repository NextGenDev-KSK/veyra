#include "Screens/HomeScreen.h"

#include "Graphics/Icons.h"
#include "Theme/Fonts.h"

namespace veyra::ui {

namespace {
struct KnobSpec { const char* label; const char* value; double v; };
const KnobSpec kKnobSpecs[] = {
    {"Bass Boost", "+4 dB", 0.33}, {"Treble", "+2 dB", 0.27},
    {"Volume Gain", "100%", 0.33}, {"Stereo Width", "100%", 0.5},
    {"Reverb", "0%", 0.0},         {"Compression", "0%", 0.0},
};

// "More Effects" tile.
class MoreEffectsCard : public GlassPanel {
protected:
    void paintContent(juce::Graphics& g) override
    {
        auto b = getLocalBounds();
        auto top = b.removeFromTop(b.getHeight() / 2);
        icons::plus(g, juce::Rectangle<float>(0, 0, 26, 26)
                            .withCentre(top.toFloat().getCentre().translated(0, 8)),
                    palette_.textSecondary);
        g.setColour(palette_.textSecondary);
        g.setFont(fonts::body(12.0f));
        g.drawText("More Effects", b, juce::Justification::centred, false);
    }
};
} // namespace

HomeScreen::HomeScreen()
{
    addAndMakeVisible(background_);
    addAndMakeVisible(topBar_);
    addAndMakeVisible(sidebar_);

    viz_.setBackdrop(&background_);
    eq_.setBackdrop(&background_);
    addAndMakeVisible(viz_);
    addAndMakeVisible(eq_);

    for (int i = 0; i < kKnobs; ++i)
    {
        auto card = std::make_unique<GlassPanel>();
        card->setBackdrop(&background_);

        auto knob = std::make_unique<Knob>();
        knob->setLabel(kKnobSpecs[i].label);
        knob->setValueText(kKnobSpecs[i].value);
        knob->setValue(kKnobSpecs[i].v);
        card->addAndMakeVisible(*knob);

        addAndMakeVisible(*card);
        knobCards_[(size_t) i] = std::move(card);
        knobs_[(size_t) i] = std::move(knob);
    }

    moreCard_ = std::make_unique<MoreEffectsCard>();
    moreCard_->setBackdrop(&background_);
    addAndMakeVisible(*moreCard_);
}

void HomeScreen::setPalette(const Palette& p)
{
    background_.setPalette(p);
    topBar_.setPalette(p);
    sidebar_.setPalette(p);
    viz_.setPalette(p);
    eq_.setPalette(p);
    for (int i = 0; i < kKnobs; ++i)
    {
        knobCards_[(size_t) i]->setPalette(p);
        knobs_[(size_t) i]->setPalette(p);
    }
    moreCard_->setPalette(p);
}

void HomeScreen::resized()
{
    auto b = getLocalBounds();
    background_.setBounds(b);
    topBar_.setBounds(b.removeFromTop(56));
    sidebar_.setBounds(b.removeFromLeft(220));

    auto main = b.reduced(24);
    viz_.setBounds(main.removeFromTop(220));
    main.removeFromTop(20);

    auto knobsRow = main.removeFromBottom(132);
    main.removeFromBottom(20);
    eq_.setBounds(main);

    const int n = kKnobs + 1;
    const int gap = 16;
    const int cw = (knobsRow.getWidth() - gap * (n - 1)) / n;
    int x = knobsRow.getX();
    for (int i = 0; i < kKnobs; ++i)
    {
        knobCards_[(size_t) i]->setBounds(x, knobsRow.getY(), cw, knobsRow.getHeight());
        knobs_[(size_t) i]->setBounds(knobCards_[(size_t) i]->getLocalBounds().reduced(8));
        x += cw + gap;
    }
    moreCard_->setBounds(x, knobsRow.getY(), cw, knobsRow.getHeight());
}

} // namespace veyra::ui
