#include "Screens/SoundLabScreen.h"

#include "Components/GlassPanel.h"
#include "Graphics/GlassBackground.h"
#include "Theme/Fonts.h"

#include <algorithm>
#include <cmath>

namespace veyra::ui {

using veyra::dsp::LabSignalParams;
using veyra::dsp::TestChannel;
using veyra::dsp::TestSignal;

namespace {
struct ToolInfo { const char* title; const char* desc; };
const ToolInfo kTool[] = {
    {"SPEAKER TEST",      "Confirm each channel plays from the correct direction."},
    {"7.1 SURROUND TEST", "Plays a positioned test tone (stereo virtualization on headphones)."},
    {"MICROPHONE TEST",   "Speak: live level plus a 10-band frequency-response spectrum."},
    {"FREQUENCY SWEEP",   "A 20 Hz to 20 kHz logarithmic sweep."},
    {"HEARING RANGE",     "Raise the frequency until you can no longer hear the tone."},
    {"POLARITY / PHASE",  "Compare in-phase against an inverted right channel."},
    {"NOISE GENERATOR",   "Continuous white or pink noise."},
};
constexpr float kHpFreqs[] = {250.0f, 500.0f, 1000.0f, 2000.0f, 4000.0f, 8000.0f};
} // namespace

// Central glass card: title + description, optional mic meter / big readout.
class SoundLabScreen::LabCard : public GlassPanel {
public:
    void setText(const char* title, const char* desc) { title_ = title; desc_ = desc; repaint(); }
    void setMeterVisible(bool v) { meterVisible_ = v; repaint(); }
    void setMeter(float level)   { level_ = level; if (meterVisible_) repaint(); }
    void setBig(juce::String s)  { big_ = std::move(s); repaint(); }
    void setBigVisible(bool v)   { bigVisible_ = v; repaint(); }
    void setBandsVisible(bool v) { bandsVisible_ = v; repaint(); }
    void setBands(const std::array<float, 10>& b) { bands_ = b; if (bandsVisible_) repaint(); }

protected:
    void paintContent(juce::Graphics& g) override
    {
        auto c = getLocalBounds().reduced(28);
        auto top = c.removeFromTop(120);
        g.setColour(palette_.textPrimary);
        g.setFont(fonts::display(26.0f));
        g.drawText(title_, top.removeFromTop(40), juce::Justification::centredTop, false);
        g.setColour(palette_.textSecondary);
        g.setFont(fonts::body(14.0f));
        g.drawFittedText(desc_, top.removeFromTop(44), juce::Justification::centredTop, 2);

        if (bigVisible_)
        {
            g.setColour(palette_.accentPrimary);
            g.setFont(fonts::mono(40.0f));
            g.drawText(big_, getLocalBounds().reduced(28).withTrimmedBottom(120),
                       juce::Justification::centred, false);
        }

        if (bandsVisible_)
        {
            auto area = getLocalBounds().reduced(40);
            area.removeFromTop(130);
            area.removeFromBottom(150);
            if (area.getHeight() > 24)
            {
                const int nb = (int) bands_.size();
                const float bw = (float) area.getWidth() / (float) nb;
                for (int b = 0; b < nb; ++b)
                {
                    const float db = bands_[(size_t) b] > 1.0e-5f
                                         ? 20.0f * std::log10(bands_[(size_t) b]) : -80.0f;
                    const float h = juce::jlimit(0.0f, 1.0f, (db + 60.0f) / 60.0f);
                    const float bh = h * (float) area.getHeight();
                    juce::Rectangle<float> bar(area.getX() + b * bw + 2.0f,
                                               area.getBottom() - bh, bw - 4.0f, bh);
                    g.setColour(palette_.accentPrimary.withAlpha(0.85f));
                    g.fillRoundedRectangle(bar, 3.0f);
                }
                g.setColour(palette_.textTertiary);
                g.setFont(fonts::mono(10.0f));
                g.drawText("31  63 125 250 500  1k  2k  4k  8k 16k",
                           area.withY(area.getBottom() + 4).withHeight(14),
                           juce::Justification::centred, false);
            }
        }

        if (meterVisible_)
        {
            auto m = getLocalBounds().reduced(28).removeFromBottom(120).removeFromTop(28)
                         .reduced(40, 6);
            g.setColour(palette_.bgInput);
            g.fillRoundedRectangle(m.toFloat(), 4.0f);
            const int segs = 32;
            const int lit = juce::jlimit(0, segs, juce::roundToInt(level_ * segs * 1.4f));
            const float sw = (float) m.getWidth() / segs;
            for (int i = 0; i < lit; ++i)
            {
                g.setColour(i > segs - 5 ? palette_.danger : palette_.success);
                g.fillRect(juce::Rectangle<float>(m.getX() + i * sw, (float) m.getY(),
                                                  sw - 2.0f, (float) m.getHeight()));
            }
        }
    }

private:
    juce::String title_, desc_, big_;
    bool  meterVisible_ = false, bigVisible_ = false, bandsVisible_ = false;
    float level_ = 0.0f;
    std::array<float, 10> bands_{};
};

// ---------------------------------------------------------------------------

SoundLabScreen::SoundLabScreen()
{
    card_ = std::make_unique<LabCard>();
    addAndMakeVisible(*card_);

    juce::StringArray names;
    for (auto& t : kTool) names.add(t.title);
    tabs_.setItems(names);
    tabs_.onChange = [this](int i) { selectTool(i); };
    addAndMakeVisible(tabs_);

    start_.setButtonText("Start Test");
    start_.onClick = [this] { startCurrentTool(); };
    stop_.setButtonText("Stop");
    stop_.onClick = [this] { engine_.stop(); };
    chL_.setButtonText("Left");      chL_.onClick = [this] { playChannel(TestChannel::Left); };
    chR_.setButtonText("Right");     chR_.onClick = [this] { playChannel(TestChannel::Right); };
    chBoth_.setButtonText("Both");   chBoth_.onClick = [this] { playChannel(TestChannel::Both); };
    phIn_.setButtonText("In Phase"); phIn_.onClick = [this] { playPhase(false); };
    phInv_.setButtonText("Inverted"); phInv_.onClick = [this] { playPhase(true); };

    noiseType_.setItems({"White", "Pink"});
    noiseType_.onChange = [this](int) { if (tool_ == 6) startCurrentTool(); };

    freq_.setSliderStyle(juce::Slider::LinearHorizontal);
    freq_.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    freq_.setRange(20.0, 20000.0);
    freq_.setSkewFactorFromMidPoint(1000.0);
    freq_.setValue(1000.0, juce::dontSendNotification);
    freq_.onValueChange = [this]
    {
        card_->setBig(juce::String(juce::roundToInt(freq_.getValue())) + " Hz");
        if (engineActiveForHearing_) playTone(freq_.getValue());
    };

    hpStart_.setButtonText("Personalize EQ");
    hpStart_.onClick = [this] { hpStartWizard(); };
    hpNext_.setButtonText("Next");
    hpNext_.onClick = [this] { hpRecordAndAdvance(); };

    hpLevel_.setSliderStyle(juce::Slider::LinearHorizontal);
    hpLevel_.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    hpLevel_.setRange(0.0, 0.15, 0.001);
    hpLevel_.setValue(0.06, juce::dontSendNotification);
    hpLevel_.onValueChange = [this]
    { if (hpActive()) playToneAt(kHpFreqs[(size_t) wizStep_], (float) hpLevel_.getValue()); };

    for (auto* b : {&start_, &stop_, &chL_, &chR_, &chBoth_, &phIn_, &phInv_, &hpStart_, &hpNext_})
        addAndMakeVisible(b);
    addAndMakeVisible(noiseType_);
    addAndMakeVisible(freq_);
    addAndMakeVisible(hpLevel_);

    selectTool(0);
}

SoundLabScreen::~SoundLabScreen() { engine_.shutdown(); }

void SoundLabScreen::setPalette(const Palette& p)
{
    palette_ = p;
    card_->setPalette(p);
    tabs_.setPalette(p);
    noiseType_.setPalette(p);
    repaint();
}

void SoundLabScreen::attachBackdrop(GlassBackground* b) { card_->setBackdrop(b); }

void SoundLabScreen::playChannel(TestChannel ch)
{
    LabSignalParams p;
    p.type = TestSignal::SineTone;
    p.frequency = 1000.0f;
    p.channel = ch;
    p.level = 0.4f;
    engine_.play(p);
}

void SoundLabScreen::playPhase(bool invert)
{
    LabSignalParams p;
    p.type = TestSignal::SineTone;
    p.frequency = 250.0f;
    p.invertRight = invert;
    p.level = 0.4f;
    engine_.play(p);
}

void SoundLabScreen::playTone(double hz)
{
    playToneAt(hz, 0.35f);
}

void SoundLabScreen::playToneAt(double hz, float level)
{
    LabSignalParams p;
    p.type = TestSignal::SineTone;
    p.frequency = (float) hz;
    p.level = level;
    engine_.play(p);
}

void SoundLabScreen::hpStartWizard()
{
    wizStep_ = 0;
    wizThresh_.fill(0.04f);
    engineActiveForHearing_ = false;
    hpLevel_.setValue(0.06, juce::dontSendNotification);
    freq_.setVisible(false); hpStart_.setVisible(false); start_.setVisible(false);
    hpLevel_.setVisible(true); hpNext_.setVisible(true);
    hpUpdateCard();
    playToneAt(kHpFreqs[0], (float) hpLevel_.getValue());
    resized();
    repaint();
}

void SoundLabScreen::hpRecordAndAdvance()
{
    if (wizStep_ < 0)
        return;
    wizThresh_[(size_t) wizStep_] = (float) hpLevel_.getValue(); // just-audible level
    ++wizStep_;
    if (wizStep_ >= kHpSteps)
    {
        hpFinish();
        return;
    }
    hpLevel_.setValue(0.06, juce::dontSendNotification);
    hpUpdateCard();
    playToneAt(kHpFreqs[(size_t) wizStep_], (float) hpLevel_.getValue());
}

void SoundLabScreen::hpFinish()
{
    engine_.stop();
    // Self-relative thresholds: the most-sensitive band is the 0 dB reference;
    // bands needing more level read as loss -> boost (half-gain rule downstream).
    float best = 1.0f;
    for (float t : wizThresh_) best = std::min(best, std::max(t, 1.0e-4f));
    std::vector<veyra::HearingPoint> pts;
    for (int i = 0; i < kHpSteps; ++i)
    {
        const float thr = std::max(wizThresh_[(size_t) i], 1.0e-4f);
        veyra::HearingPoint hp;
        hp.freq = kHpFreqs[i];
        hp.lossDb = 20.0f * std::log10(thr / best);
        pts.push_back(hp);
    }
    const auto bands = veyra::personalizeFromHearingTest(pts);

    wizStep_ = -1;
    hpLevel_.setVisible(false); hpNext_.setVisible(false);
    freq_.setVisible(true); hpStart_.setVisible(true); start_.setVisible(true);
    if (onPersonalized)
        onPersonalized(bands);
    card_->setBig({});
    card_->setText("HEARING RANGE",
                   bands.empty() ? "All bands even, no correction needed."
                                 : "Personalized EQ applied from your hearing profile.");
    resized();
    repaint();
}

void SoundLabScreen::hpUpdateCard()
{
    card_->setText("HEARING TEST",
                   "Lower the level until the tone just disappears, then Next.");
    card_->setBig(juce::String(juce::roundToInt(kHpFreqs[(size_t) wizStep_])) + " Hz  ("
                  + juce::String(wizStep_ + 1) + "/" + juce::String(kHpSteps) + ")");
}

void SoundLabScreen::startCurrentTool()
{
    LabSignalParams p;
    p.level = 0.4f;
    switch (tool_)
    {
    case 1: p.type = TestSignal::SineTone; p.frequency = 600.0f; break;       // surround (simplified)
    case 3: p.type = TestSignal::Sweep; p.sweepStartHz = 20.0f;
            p.sweepEndHz = 20000.0f; p.sweepSeconds = 6.0f; break;            // sweep
    case 4: engineActiveForHearing_ = true; playTone(freq_.getValue()); return; // hearing
    case 6: p.type = noiseType_.selectedIndex() == 1 ? TestSignal::PinkNoise
                                                     : TestSignal::WhiteNoise; break;
    default: return;
    }
    engine_.play(p);
}

void SoundLabScreen::selectTool(int i)
{
    tool_ = juce::jlimit(0, kTools - 1, i);
    engineActiveForHearing_ = false;
    wizStep_ = -1; // cancel any in-progress hearing test
    engine_.stop();
    card_->setText(kTool[tool_].title, kTool[tool_].desc);
    card_->setMeterVisible(tool_ == 2);
    card_->setBandsVisible(tool_ == 2); // live FR spectrum during the Mic Test
    card_->setBigVisible(tool_ == 4);
    if (tool_ == 4)
        card_->setBig(juce::String(juce::roundToInt(freq_.getValue())) + " Hz");

    // Control visibility per tool.
    chL_.setVisible(tool_ == 0); chR_.setVisible(tool_ == 0); chBoth_.setVisible(tool_ == 0);
    phIn_.setVisible(tool_ == 5); phInv_.setVisible(tool_ == 5);
    noiseType_.setVisible(tool_ == 6);
    freq_.setVisible(tool_ == 4);
    hpStart_.setVisible(tool_ == 4);
    hpNext_.setVisible(false);
    hpLevel_.setVisible(false);
    start_.setVisible(tool_ == 1 || tool_ == 3 || tool_ == 4 || tool_ == 6);
    stop_.setVisible(tool_ != 2);

    if (tool_ == 2) { engine_.start(); startTimerHz(30); } else { stopTimer(); }
    resized();
    repaint();
}

void SoundLabScreen::timerCallback()
{
    if (tool_ == 2)
    {
        card_->setMeter(engine_.inputLevel());
        std::array<float, 10> bands{};
        for (int b = 0; b < SoundLabEngine::kBands && b < 10; ++b)
            bands[(size_t) b] = engine_.bandLevel(b);
        card_->setBands(bands);
    }
}

void SoundLabScreen::visibilityChanged()
{
    if (!isVisible())
    {
        stopTimer();
        engine_.shutdown();
        engineActiveForHearing_ = false;
        if (hpActive()) selectTool(tool_); // cancel an in-progress hearing test
    }
}

void SoundLabScreen::paint(juce::Graphics&) {} // cards/backdrop paint themselves

void SoundLabScreen::resized()
{
    auto b = getLocalBounds().reduced(24);
    tabs_.setBounds(b.removeFromTop(40));
    b.removeFromTop(16);
    card_->setBounds(b);

    // Action row sits in the lower third of the card; Stop sits below it.
    const auto card = card_->getBounds();
    const int cx = card.getCentreX();
    const int rowY = card.getCentreY() + 40;
    auto centred = [&](juce::Component& c, int w, int h, int y)
    { c.setBounds(cx - w / 2, y, w, h); };

    if (tool_ == 0) // Speaker: Left | Both | Right
    {
        const int w = 110, gap = 12, total = w * 3 + gap * 2;
        int x = cx - total / 2;
        chL_.setBounds(x, rowY, w, 40);    x += w + gap;
        chBoth_.setBounds(x, rowY, w, 40); x += w + gap;
        chR_.setBounds(x, rowY, w, 40);
    }
    else if (tool_ == 5) // Polarity: In Phase | Inverted
    {
        const int w = 130, gap = 12, total = w * 2 + gap;
        int x = cx - total / 2;
        phIn_.setBounds(x, rowY, w, 40); x += w + gap;
        phInv_.setBounds(x, rowY, w, 40);
    }
    else if (tool_ == 4) // Hearing
    {
        if (hpActive()) // wizard: level slider + Next
        {
            hpLevel_.setBounds(cx - 180, rowY - 8, 360, 24);
            centred(hpNext_, 160, 38, rowY + 28);
        }
        else // range test (freq slider + Start) + Personalize entry
        {
            freq_.setBounds(cx - 180, rowY - 16, 360, 24);
            centred(start_, 160, 38, rowY + 18);
            centred(hpStart_, 180, 34, rowY + 64);
        }
    }
    else if (tool_ == 6) // Noise: White|Pink + Start
    {
        centred(noiseType_, 200, 30, rowY - 6);
        centred(start_, 160, 38, rowY + 34);
    }
    else if (tool_ == 1 || tool_ == 3) // Surround / Sweep: Start
    {
        centred(start_, 160, 40, rowY);
    }

    if (stop_.isVisible())
        centred(stop_, 100, 28, card.getBottom() - 56);
}

} // namespace veyra::ui
