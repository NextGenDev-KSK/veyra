#include "Screens/SoundLabScreen.h"

#include "Components/GlassPanel.h"
#include "Components/Knob.h"
#include "Components/ToggleSwitch.h"
#include "Graphics/GlassBackground.h"
#include "Theme/Fonts.h"

namespace veyra::ui {

namespace {
// Knob value (0..1) <-> engineering units.
float  lufsFrom01(double v) { return (float) (-23.0 + v * 14.0); }   // -23 .. -9
double lufsTo01(float l)    { return juce::jlimit(0.0, 1.0, (l + 23.0) / 14.0); }
float  minsFrom01(double v) { return (float) (v * 120.0); }          // 0 .. 120
double minsTo01(float m)    { return juce::jlimit(0.0, 1.0, m / 120.0); }
float  fadeFrom01(double v) { return (float) (5.0 + v * 55.0); }     // 5 .. 60
double fadeTo01(float s)    { return juce::jlimit(0.0, 1.0, (s - 5.0) / 55.0); }
} // namespace

class SoundLabScreen::Card : public GlassPanel {
public:
    Card()
    {
        night_.setLabel("Night Mode");
        night_.setDimWhenZero(true);
        night_.onChange = [this](double v)
        {
            cfg_.nightModeAmount = (float) v;
            night_.setValueText(juce::String(juce::roundToInt(v * 100.0)) + "%");
            emit();
        };
        addAndMakeVisible(night_);

        match_.onClick = [this] { cfg_.loudnessMatch = match_.getToggleState(); emit(); };
        addAndMakeVisible(match_);

        target_.setLabel("Target");
        target_.onChange = [this](double v)
        {
            cfg_.targetLufs = lufsFrom01(v);
            target_.setValueText(juce::String(cfg_.targetLufs, 0) + " LUFS");
            emit();
        };
        addAndMakeVisible(target_);

        sleep_.onClick = [this] { cfg_.sleepTimerEnabled = sleep_.getToggleState(); emit(); };
        addAndMakeVisible(sleep_);

        minutes_.setLabel("Timer");
        minutes_.onChange = [this](double v)
        {
            cfg_.sleepTimerMinutes = minsFrom01(v);
            minutes_.setValueText(juce::String(juce::roundToInt(cfg_.sleepTimerMinutes)) + " min");
            emit();
        };
        addAndMakeVisible(minutes_);

        fade_.setLabel("Fade");
        fade_.onChange = [this](double v)
        {
            cfg_.sleepFadeSeconds = fadeFrom01(v);
            fade_.setValueText(juce::String(juce::roundToInt(cfg_.sleepFadeSeconds)) + " s");
            emit();
        };
        addAndMakeVisible(fade_);
    }

    std::function<void(const veyra::LoudnessConfig&)> onLoudnessChanged;

    void setPalette(const Palette& p) override
    {
        GlassPanel::setPalette(p);
        for (Knob* k : {&night_, &target_, &minutes_, &fade_}) k->setPalette(p);
        match_.setPalette(p);
        sleep_.setPalette(p);
    }

    void setLoudness(const veyra::LoudnessConfig& l)
    {
        cfg_ = l;
        night_.setValue(l.nightModeAmount);
        night_.setValueText(juce::String(juce::roundToInt(l.nightModeAmount * 100.0f)) + "%");
        match_.setToggleState(l.loudnessMatch, juce::dontSendNotification);
        target_.setValue(lufsTo01(l.targetLufs));
        target_.setValueText(juce::String(l.targetLufs, 0) + " LUFS");
        sleep_.setToggleState(l.sleepTimerEnabled, juce::dontSendNotification);
        minutes_.setValue(minsTo01(l.sleepTimerMinutes));
        minutes_.setValueText(juce::String(juce::roundToInt(l.sleepTimerMinutes)) + " min");
        fade_.setValue(fadeTo01(l.sleepFadeSeconds));
        fade_.setValueText(juce::String(juce::roundToInt(l.sleepFadeSeconds)) + " s");
        repaint();
    }

    void resized() override
    {
        auto c = getLocalBounds().reduced(kPad);
        c.removeFromTop(kHeader);
        c.removeFromTop(kSub);                     // "Loudness"
        auto tA = c.removeFromTop(kToggle);
        match_.setBounds(tA.getX() + 150, tA.getY(), 46, 24);
        auto kA = c.removeFromTop(kKnob);
        night_.setBounds(kA.getX(), kA.getY(), 96, kKnob);
        target_.setBounds(kA.getX() + 130, kA.getY(), 96, kKnob);

        c.removeFromTop(kSub);                     // "Sleep Timer"
        auto tB = c.removeFromTop(kToggle);
        sleep_.setBounds(tB.getX() + 150, tB.getY(), 46, 24);
        auto kB = c.removeFromTop(kKnob);
        minutes_.setBounds(kB.getX(), kB.getY(), 96, kKnob);
        fade_.setBounds(kB.getX() + 130, kB.getY(), 96, kKnob);
    }

protected:
    void paintContent(juce::Graphics& g) override
    {
        auto c = getLocalBounds().reduced(kPad);
        g.setColour(palette_.textPrimary);
        g.setFont(fonts::display(20.0f));
        g.drawText("SOUND LAB", c.removeFromTop(kHeader), juce::Justification::centredLeft, false);

        auto subhead = [&](juce::Rectangle<int> r, const char* t)
        {
            g.setColour(palette_.accentSecondary);
            g.setFont(fonts::mono(11.0f, true));
            g.drawText(t, r, juce::Justification::bottomLeft, false);
        };
        auto rowLabel = [&](juce::Rectangle<int> r, const char* t)
        {
            g.setColour(palette_.textSecondary);
            g.setFont(fonts::body(13.0f));
            g.drawText(t, r.withWidth(150), juce::Justification::centredLeft, false);
        };

        subhead(c.removeFromTop(kSub), "LOUDNESS");
        rowLabel(c.removeFromTop(kToggle), "Loudness Match");
        c.removeFromTop(kKnob);
        subhead(c.removeFromTop(kSub), "SLEEP TIMER");
        rowLabel(c.removeFromTop(kToggle), "Enable");
        // knob labels are drawn by the knobs themselves
    }

private:
    void emit() { if (onLoudnessChanged) onLoudnessChanged(cfg_); repaint(); }

    static constexpr int kPad = 24, kHeader = 28, kSub = 24, kToggle = 30, kKnob = 96;

    veyra::LoudnessConfig cfg_;
    Knob         night_, target_, minutes_, fade_;
    ToggleSwitch match_, sleep_;
};

// ---------------------------------------------------------------------------

SoundLabScreen::SoundLabScreen()
{
    card_ = std::make_unique<Card>();
    card_->onLoudnessChanged = [this](const veyra::LoudnessConfig& l) { if (onLoudnessChanged) onLoudnessChanged(l); };
    addAndMakeVisible(*card_);
}

SoundLabScreen::~SoundLabScreen() = default;

void SoundLabScreen::setPalette(const Palette& p)       { card_->setPalette(p); }
void SoundLabScreen::attachBackdrop(GlassBackground* b) { card_->setBackdrop(b); }
void SoundLabScreen::setLoudness(const veyra::LoudnessConfig& l) { card_->setLoudness(l); }
void SoundLabScreen::resized() { card_->setBounds(getLocalBounds().reduced(24)); }

} // namespace veyra::ui
