#include "Screens/HomeScreen.h"

#include "Graphics/GlassBackground.h"
#include "Graphics/Icons.h"
#include "Theme/Fonts.h"

namespace veyra::ui {

namespace {
struct KnobSpec { const char* label; double v; };
const KnobSpec kKnobSpecs[] = {
    {"Bass Boost", 0.33}, {"Treble", 0.27},
    {"Volume Gain", 0.33}, {"Stereo Width", 0.5},
    {"Reverb", 0.0},      {"Compression", 0.0},
};

juce::String dbText(float db)  { return (db >= 0 ? "+" : "") + juce::String(juce::roundToInt(db)) + " dB"; }
juce::String pctText(float fr) { return juce::String(juce::roundToInt(fr * 100.0f)) + "%"; }

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
    addAndMakeVisible(viz_);
    addAndMakeVisible(eq_);

    for (int i = 0; i < kKnobs; ++i)
    {
        auto card = std::make_unique<GlassPanel>();
        card->setElevated(true); // knob cards sit a tier above the big panels

        auto knob = std::make_unique<Knob>();
        knob->setLabel(kKnobSpecs[i].label);
        knob->setValue(kKnobSpecs[i].v);
        if (i == 4 || i == 5) // Reverb, Compression: show an "off" look at zero
            knob->setDimWhenZero(true);
        if (i == 2)           // Volume Gain: danger zone past ~200%
            knob->setDangerThreshold(0.66f);
        knob->onChange = [this, i](double v) { onKnobChanged(i, v); };
        card->addAndMakeVisible(*knob);

        addAndMakeVisible(*card);
        knobCards_[(size_t) i] = std::move(card);
        knobs_[(size_t) i] = std::move(knob);
    }

    moreCard_ = std::make_unique<MoreEffectsCard>();
    addAndMakeVisible(*moreCard_);

    eq_.onBandChanged = [this](int i, float db)
    {
        enh_.eqBandsDb[(size_t) i] = db;
        if (onEnhancementChanged)
            onEnhancementChanged(enh_);
    };

    seedFromControls();
    applyEnhancement(enh_); // normalise displayed values/text from enh_
}

void HomeScreen::setPalette(const Palette& p)
{
    viz_.setPalette(p);
    eq_.setPalette(p);
    for (int i = 0; i < kKnobs; ++i)
    {
        knobCards_[(size_t) i]->setPalette(p);
        knobs_[(size_t) i]->setPalette(p);
    }
    moreCard_->setPalette(p);
}

void HomeScreen::attachBackdrop(GlassBackground* bg)
{
    viz_.setBackdrop(bg);
    eq_.setBackdrop(bg);
    for (auto& card : knobCards_)
        card->setBackdrop(bg);
    moreCard_->setBackdrop(bg);
}

void HomeScreen::onKnobChanged(int index, double v01)
{
    auto& e = enh_;
    switch (index)
    {
    case 0: e.bassBoostDb       = (float) (v01 * 12.0); knobs_[0]->setValueText(dbText(e.bassBoostDb)); break;
    case 1: e.trebleDb          = (float) (v01 * 12.0); knobs_[1]->setValueText(dbText(e.trebleDb));    break;
    case 2: e.volumeGain        = (float) (v01 * 3.0);  knobs_[2]->setValueText(pctText(e.volumeGain)); break;
    case 3: e.stereoWidth       = (float) (v01 * 2.0);  knobs_[3]->setValueText(pctText(e.stereoWidth)); break;
    case 4: e.reverbAmount      = (float) v01;          knobs_[4]->setValueText(pctText(e.reverbAmount)); break;
    case 5: e.compressionAmount = (float) v01;          knobs_[5]->setValueText(pctText(e.compressionAmount)); break;
    default: return;
    }
    if (onEnhancementChanged)
        onEnhancementChanged(enh_);
}

void HomeScreen::applyEnhancement(const EnhancementConfig& e)
{
    enh_ = e;
    knobs_[0]->setValue(e.bassBoostDb / 12.0);  knobs_[0]->setValueText(dbText(e.bassBoostDb));
    knobs_[1]->setValue(e.trebleDb / 12.0);     knobs_[1]->setValueText(dbText(e.trebleDb));
    knobs_[2]->setValue(e.volumeGain / 3.0);    knobs_[2]->setValueText(pctText(e.volumeGain));
    knobs_[3]->setValue(e.stereoWidth / 2.0);   knobs_[3]->setValueText(pctText(e.stereoWidth));
    knobs_[4]->setValue(e.reverbAmount);        knobs_[4]->setValueText(pctText(e.reverbAmount));
    knobs_[5]->setValue(e.compressionAmount);   knobs_[5]->setValueText(pctText(e.compressionAmount));

    for (int i = 0; i < EqualizerCard::kBands; ++i)
        eq_.setBandGain(i, e.eqBandsDb[(size_t) i]);
}

void HomeScreen::seedFromControls()
{
    enh_.bassBoostDb       = (float) (knobs_[0]->getValue() * 12.0);
    enh_.trebleDb          = (float) (knobs_[1]->getValue() * 12.0);
    enh_.volumeGain        = (float) (knobs_[2]->getValue() * 3.0);
    enh_.stereoWidth       = (float) (knobs_[3]->getValue() * 2.0);
    enh_.reverbAmount      = (float) knobs_[4]->getValue();
    enh_.compressionAmount = (float) knobs_[5]->getValue();
    for (int i = 0; i < EqualizerCard::kBands; ++i)
        enh_.eqBandsDb[(size_t) i] = eq_.bandGain(i);
}

void HomeScreen::resized()
{
    auto main = getLocalBounds().reduced(24);
    viz_.setBounds(main.removeFromTop(150)); // shorter — more room for controls
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
