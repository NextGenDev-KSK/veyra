#include "Screens/HomeScreen.h"

#include "Graphics/GlassBackground.h"
#include "Graphics/Icons.h"
#include "Theme/Fonts.h"

#include "veyra/AutoEq.h"

#include <algorithm>
#include <string>

namespace veyra::ui {

namespace {
// AutoEQ headphone corrections embedded as BinaryData (resources/autoeq/*.txt).
std::vector<veyra::AutoEqProfile> loadEmbeddedAutoEq()
{
    std::vector<veyra::AutoEqProfile> out;
    for (int i = 0; i < BinaryData::namedResourceListSize; ++i)
    {
        const juce::String fn = BinaryData::originalFilenames[i];
        if (!fn.endsWithIgnoreCase(".txt"))
            continue;
        int sz = 0;
        const char* d = BinaryData::getNamedResource(BinaryData::namedResourceList[i], sz);
        if (!d || sz <= 0)
            continue;
        if (auto p = veyra::parseAutoEq(fn.dropLastCharacters(4).toStdString(), std::string(d, (size_t) sz)))
            out.push_back(std::move(*p));
    }
    std::sort(out.begin(), out.end(),
              [](const veyra::AutoEqProfile& a, const veyra::AutoEqProfile& b) { return a.name < b.name; });
    return out;
}

struct KnobSpec { const char* label; double v; };
const KnobSpec kKnobSpecs[] = {
    {"Bass Boost", 0.33}, {"Treble", 0.27},
    {"Volume Gain", 0.33}, {"Stereo Width", 0.5},
    {"Reverb", 0.0},      {"Compression", 0.0},
};

juce::String dbText(float db)  { return (db >= 0 ? "+" : "") + juce::String(juce::roundToInt(db)) + " dB"; }
juce::String pctText(float fr) { return juce::String(juce::roundToInt(fr * 100.0f)) + "%"; }

// "More Effects" tile — opens the effects rack overview.
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

    // Per-feature info hints (hover → what it is, what it does, shortcut).
    // ASCII only: const char* literals go through juce::String(const char*).
    static const char* kKnobTips[kKnobs] = {
        "Bass Boost: lifts low frequencies for a fuller low end (low shelf, up to +12 dB).",
        "Treble: lifts highs for air and detail (high shelf, up to +12 dB).",
        "Volume Gain: output pre-amp up to 300%. Hotkeys: Ctrl+Alt+Up / Ctrl+Alt+Down.",
        "Stereo Width: widens or narrows the stereo image (mid/side), 0 to 200%.",
        "Reverb: adds room ambience, mixed with the dry signal.",
        "Compression: evens out loud and quiet parts for a steadier level.",
    };
    for (int i = 0; i < kKnobs; ++i)
    {
        knobInfo_[(size_t) i].setTooltip(kKnobTips[i]);
        addAndMakeVisible(knobInfo_[(size_t) i]);
    }
    eqInfo_.setTooltip("Equalizer: 10-band graphic EQ, or switch to Parametric for draggable "
                       "bell / shelf / notch / HP / LP nodes (drag = freq+gain, wheel = Q).");
    vizInfo_.setTooltip("Visualizer: live spectrum from your audio. Pick a mode (top-right) "
                        "or open fullscreen.");
    addAndMakeVisible(eqInfo_);
    addAndMakeVisible(vizInfo_);

    eq_.onBandChanged = [this](int i, float db)
    {
        enh_.eqBandsDb[(size_t) i] = db;
        if (onEnhancementChanged)
            onEnhancementChanged(enh_);
    };

    eq_.onModeChanged = [this](bool parametric)
    {
        enh_.eqMode = parametric ? "parametric" : "graphic";
        if (parametric && enh_.parametricBands.empty())
        {
            // Seed parametric mode from the current 10 graphic bands (bells at the
            // standard centres) so the editor opens on the user's existing curve.
            static constexpr float kCentres[10] =
                {31.25f, 62.5f, 125.f, 250.f, 500.f, 1000.f, 2000.f, 4000.f, 8000.f, 16000.f};
            for (int b = 0; b < 10; ++b)
                enh_.parametricBands.push_back(
                    veyra::ParametricBand{true, 0, kCentres[b], enh_.eqBandsDb[(size_t) b], 1.41f});
        }
        eq_.setParametricBands(enh_.parametricBands);
        if (onEnhancementChanged)
            onEnhancementChanged(enh_);
    };

    eq_.onParametricChanged = [this](const std::vector<veyra::ParametricBand>& bands)
    {
        enh_.parametricBands = bands;
        if (onEnhancementChanged)
            onEnhancementChanged(enh_);
    };

    // AutoEQ: pick a headphone -> load its correction into the parametric EQ.
    eq_.setAutoEqProfiles(loadEmbeddedAutoEq());
    eq_.onAutoEqSelected = [this](const std::vector<veyra::ParametricBand>& bands)
    {
        enh_.parametricBands = bands;
        enh_.eqMode = "parametric";
        eq_.setParametricBands(bands);
        eq_.setMode(true);
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
        knobInfo_[(size_t) i].setPalette(p);
    }
    eqInfo_.setPalette(p);
    vizInfo_.setPalette(p);
}

void HomeScreen::attachBackdrop(GlassBackground* bg)
{
    viz_.setBackdrop(bg);
    eq_.setBackdrop(bg);
    for (auto& card : knobCards_)
        card->setBackdrop(bg);
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
    eq_.setParametricBands(e.parametricBands);
    eq_.setMode(e.eqMode == "parametric");
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
    eqInfo_.setBounds(eq_.getX() + 128, eq_.getY() + 24, 15, 15);
    vizInfo_.setBounds(viz_.getRight() - 172, viz_.getY() + 12, 15, 15);

    const int gap = 16;
    const int cw = (knobsRow.getWidth() - gap * (kKnobs - 1)) / kKnobs;
    int x = knobsRow.getX();
    for (int i = 0; i < kKnobs; ++i)
    {
        knobCards_[(size_t) i]->setBounds(x, knobsRow.getY(), cw, knobsRow.getHeight());
        knobs_[(size_t) i]->setBounds(knobCards_[(size_t) i]->getLocalBounds().reduced(8));
        knobInfo_[(size_t) i].setBounds(x + cw - 22, knobsRow.getY() + 6, 14, 14);
        x += cw + gap;
    }
}

} // namespace veyra::ui
