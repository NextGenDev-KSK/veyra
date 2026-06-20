#include "Screens/GamerModeScreen.h"

#include "Components/GlassPanel.h"
#include "Components/SegmentedControl.h"
#include "Graphics/GlassBackground.h"
#include "Theme/Fonts.h"

namespace veyra::ui {

namespace {
void styleSlider(juce::Slider& s)
{
    s.setSliderStyle(juce::Slider::LinearHorizontal);
    s.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    s.setRange(0.0, 1.0);
}
juce::String pct(double v) { return juce::String(juce::roundToInt(v * 100.0)) + "%"; }
} // namespace

// ===========================================================================
// Sound Tracker
// ===========================================================================
class GamerModeScreen::TrackerCard : public GlassPanel {
public:
    std::function<void(const veyra::GamerModeConfig&)> onChanged;

    TrackerCard()
    {
        style_.setItems({"Competitive", "Rich"});
        style_.onChange = [this](int i) { cfg_.radarMode = i; emit(); };
        addAndMakeVisible(style_);

        styleSlider(sens_);
        sens_.onValueChange = [this] { cfg_.sensitivity = (float) sens_.getValue(); emit(); };
        addAndMakeVisible(sens_);

        auto det = [this](ToggleSwitch& t, bool veyra::GamerModeConfig::* f)
        {
            ToggleSwitch* tp = &t;
            t.onClick = [this, tp, f] { cfg_.*f = tp->getToggleState(); emit(); };
            addAndMakeVisible(t);
        };
        det(foot_,   &veyra::GamerModeConfig::footsteps);
        det(guns_,   &veyra::GamerModeConfig::gunshots);
        det(voices_, &veyra::GamerModeConfig::voices);
    }

    void setPalette(const Palette& p) override
    {
        GlassPanel::setPalette(p);
        style_.setPalette(p);
        foot_.setPalette(p); guns_.setPalette(p); voices_.setPalette(p);
    }

    void setGamer(const veyra::GamerModeConfig& g)
    {
        cfg_ = g;
        style_.setSelectedIndex(g.radarMode < 2 ? g.radarMode : 0, false);
        sens_.setValue(g.sensitivity, juce::dontSendNotification);
        foot_.setToggleState(g.footsteps, juce::dontSendNotification);
        guns_.setToggleState(g.gunshots, juce::dontSendNotification);
        voices_.setToggleState(g.voices, juce::dontSendNotification);
        repaint();
    }

    void setEnabledFromHeader(bool on) { cfg_.enabled = on; emit(); }

    void resized() override
    {
        auto c = getLocalBounds().reduced(kPad);
        c.removeFromTop(24);            // title row
        c.removeFromTop(96);            // radar preview (painted)
        style_.setBounds(c.removeFromTop(30));
        c.removeFromTop(8);
        sens_.setBounds(c.removeFromTop(24));
        c.removeFromTop(10);
        auto row = c.removeFromTop(24);
        const int sw = 42;
        foot_.setBounds(row.getX() + 70, row.getY(), sw, 22);
        guns_.setBounds(row.getX() + 195, row.getY(), sw, 22);
        voices_.setBounds(row.getX() + 320, row.getY(), sw, 22);
    }

protected:
    void paintContent(juce::Graphics& g) override
    {
        auto c = getLocalBounds().reduced(kPad);
        auto title = c.removeFromTop(24);
        g.setColour(palette_.textPrimary);
        g.setFont(fonts::display(16.0f));
        g.drawText("SOUND TRACKER", title, juce::Justification::centredLeft, false);
        // ACTIVE / IDLE badge
        const bool on = cfg_.enabled;
        g.setColour(on ? palette_.success : palette_.textTertiary);
        g.setFont(fonts::mono(10.0f, true));
        g.drawText(on ? juce::String("ACTIVE") : juce::String("IDLE"),
                   title, juce::Justification::centredRight, false);

        // Radar preview.
        auto radar = c.removeFromTop(96).toFloat();
        const auto ctr = radar.getCentre();
        const float r = juce::jmin(radar.getWidth(), radar.getHeight()) * 0.42f;
        g.setColour(palette_.strokeHover);
        g.drawEllipse(ctr.x - r, ctr.y - r, r * 2, r * 2, 1.0f);
        g.drawEllipse(ctr.x - r * 0.5f, ctr.y - r * 0.5f, r, r, 1.0f);
        g.setColour(palette_.accentPrimary);
        g.fillEllipse(ctr.x - 3, ctr.y - 3, 6, 6); // you
        if (on)
        {
            g.setColour(palette_.warning);
            g.fillEllipse(ctr.x + r * 0.45f, ctr.y - r * 0.35f, 5, 5);
            g.setColour(palette_.success);
            g.fillEllipse(ctr.x - r * 0.5f, ctr.y + r * 0.2f, 5, 5);
        }

        c.removeFromTop(30 + 8); // style seg
        auto sl = c.removeFromTop(24); // sensitivity label/value
        g.setColour(palette_.textSecondary);
        g.setFont(fonts::body(12.0f));
        g.drawText("Sensitivity", sl.removeFromLeft(80), juce::Justification::centredLeft, false);
        g.setColour(palette_.textTertiary);
        g.setFont(fonts::mono(11.0f));
        g.drawText(pct(sens_.getValue()), sl.removeFromRight(48), juce::Justification::centredRight, false);

        c.removeFromTop(10);
        auto row = c.removeFromTop(24);
        auto lbl = [&](int x, const char* t)
        {
            g.setColour(palette_.textTertiary);
            g.setFont(fonts::body(11.0f));
            g.drawText(t, juce::Rectangle<int>(row.getX() + x, row.getY(), 64, 22),
                       juce::Justification::centredLeft, false);
        };
        lbl(0, "Steps"); lbl(120, "Guns"); lbl(250, "Voice");
    }

private:
    void emit() { if (onChanged) onChanged(cfg_); repaint(); }
    static constexpr int kPad = 20;

    veyra::GamerModeConfig cfg_;
    SegmentedControl style_;
    juce::Slider     sens_;
    ToggleSwitch     foot_, guns_, voices_;
};

// ===========================================================================
// Spatial Audio
// ===========================================================================
class GamerModeScreen::SpatialCard : public GlassPanel {
public:
    std::function<void(const veyra::SpatialConfig&)> onChanged;
    std::function<void()> onTest;

    SpatialCard()
    {
        hrtf_.setItems({"Cinematic", "Competitive"});
        hrtf_.onChange = [this](int i) { cfg_.mode = i + 1; cfg_.enabled = true; emit(); };
        addAndMakeVisible(hrtf_);

        headset_.onClick = [this] { cfg_.enabled = headset_.getToggleState(); emit(); };
        addAndMakeVisible(headset_);

        styleSlider(intensity_);
        intensity_.onValueChange = [this] { cfg_.virtualization = (float) intensity_.getValue(); emit(); };
        addAndMakeVisible(intensity_);

        styleSlider(crossfeed_);
        crossfeed_.onValueChange = [this] { cfg_.crossfeed = (float) crossfeed_.getValue(); emit(); };
        addAndMakeVisible(crossfeed_);

        test_.setButtonText("Test in Sound Lab");
        test_.onClick = [this] { if (onTest) onTest(); };
        addAndMakeVisible(test_);
    }

    void setPalette(const Palette& p) override
    {
        GlassPanel::setPalette(p);
        hrtf_.setPalette(p);
        headset_.setPalette(p);
    }

    void setSpatial(const veyra::SpatialConfig& s)
    {
        cfg_ = s;
        hrtf_.setSelectedIndex(s.mode >= 1 ? s.mode - 1 : 0, false);
        headset_.setToggleState(s.enabled, juce::dontSendNotification);
        intensity_.setValue(s.virtualization, juce::dontSendNotification);
        crossfeed_.setValue(s.crossfeed, juce::dontSendNotification);
        repaint();
    }

    void resized() override
    {
        auto c = getLocalBounds().reduced(kPad);
        c.removeFromTop(24);                 // title
        c.removeFromTop(18);                 // "HRTF Mode" label
        hrtf_.setBounds(c.removeFromTop(30));
        c.removeFromTop(14);
        auto h = c.removeFromTop(24);        // headset row
        headset_.setBounds(h.removeFromRight(46).withSizeKeepingCentre(46, 22));
        c.removeFromTop(8);
        c.removeFromTop(16);                 // intensity label
        intensity_.setBounds(c.removeFromTop(22));
        c.removeFromTop(10);
        c.removeFromTop(16);                 // crossfeed label
        crossfeed_.setBounds(c.removeFromTop(22));
        test_.setBounds(getLocalBounds().reduced(kPad).removeFromBottom(26).removeFromRight(150));
    }

protected:
    void paintContent(juce::Graphics& g) override
    {
        auto c = getLocalBounds().reduced(kPad);
        g.setColour(palette_.textPrimary);
        g.setFont(fonts::display(16.0f));
        g.drawText("SPATIAL AUDIO", c.removeFromTop(24), juce::Justification::centredLeft, false);

        auto label = [&](juce::Rectangle<int> r, const char* t, double v = -1.0)
        {
            g.setColour(palette_.textSecondary);
            g.setFont(fonts::body(12.0f));
            g.drawText(t, r, juce::Justification::centredLeft, false);
            if (v >= 0.0)
            {
                g.setColour(palette_.textTertiary);
                g.setFont(fonts::mono(11.0f));
                g.drawText(pct(v), r, juce::Justification::centredRight, false);
            }
        };
        label(c.removeFromTop(18), "HRTF Mode");
        c.removeFromTop(30 + 14);
        label(c.removeFromTop(24), "Virtual Headset Mode");
        c.removeFromTop(8);
        label(c.removeFromTop(16), "Intensity", intensity_.getValue());
        c.removeFromTop(22 + 10);
        label(c.removeFromTop(16), "Crossfeed", crossfeed_.getValue());
    }

private:
    void emit() { if (onChanged) onChanged(cfg_); repaint(); }
    static constexpr int kPad = 20;

    veyra::SpatialConfig cfg_;
    SegmentedControl hrtf_;
    ToggleSwitch     headset_;
    juce::Slider     intensity_, crossfeed_;
    juce::TextButton test_;
};

// ===========================================================================
// Voice & Microphone
// ===========================================================================
class GamerModeScreen::VoiceCard : public GlassPanel {
public:
    std::function<void(const veyra::VoiceConfig&)> onChanged;

    VoiceCard()
    {
        profile_.addItem("Gaming Chat", 1);
        profile_.addItem("Streaming", 2);
        profile_.addItem("Podcast", 3);
        profile_.addItem("Meeting", 4);
        profile_.onChange = [this]
        {
            switch (profile_.getSelectedId())
            {
            case 2: cfg_.profile = "streaming"; break;
            case 3: cfg_.profile = "podcast";   break;
            case 4: cfg_.profile = "meeting";   break;
            default: cfg_.profile = "gaming";   break;
            }
            emit();
        };
        addAndMakeVisible(profile_);

        wire(rnnoise_,  [this](bool on) { cfg_.noiseSuppression = on ? 0.6f : 0.0f; });
        wire(gate_,     [this](bool on) { cfg_.noiseGate = on; });
        wire(aec_,      [this](bool on) { cfg_.echoCancel = on; });
        wire(enhancer_, [this](bool on) { cfg_.presenceDb = on ? 3.0f : 0.0f; });
        wire(sidetone_, [this](bool on) { cfg_.sideToneLevel = on ? 0.3f : 0.0f; });
    }

    void setPalette(const Palette& p) override
    {
        GlassPanel::setPalette(p);
        for (ToggleSwitch* t : {&rnnoise_, &gate_, &aec_, &enhancer_, &sidetone_})
            t->setPalette(p);
    }

    void setVoice(const veyra::VoiceConfig& v)
    {
        cfg_ = v;
        const int id = v.profile == "streaming" ? 2 : v.profile == "podcast" ? 3
                       : v.profile == "meeting" ? 4 : 1;
        profile_.setSelectedId(id, juce::dontSendNotification);
        rnnoise_.setToggleState(v.noiseSuppression > 0.001f, juce::dontSendNotification);
        gate_.setToggleState(v.noiseGate, juce::dontSendNotification);
        aec_.setToggleState(v.echoCancel, juce::dontSendNotification);
        enhancer_.setToggleState(v.presenceDb > 0.001f, juce::dontSendNotification);
        sidetone_.setToggleState(v.sideToneLevel > 0.001f, juce::dontSendNotification);
        repaint();
    }

    void resized() override
    {
        auto c = getLocalBounds().reduced(kPad);
        c.removeFromTop(24);                  // title
        profile_.setBounds(c.removeFromTop(28));
        c.removeFromTop(12);
        ToggleSwitch* rows[] = {&rnnoise_, &gate_, &aec_, &enhancer_, &sidetone_};
        for (auto* t : rows)
        {
            auto r = c.removeFromTop(26);
            t->setBounds(r.removeFromRight(44).withSizeKeepingCentre(44, 22));
        }
        c.removeFromTop(6);
        c.removeFromTop(16);                  // input meter (painted)
    }

protected:
    void paintContent(juce::Graphics& g) override
    {
        auto c = getLocalBounds().reduced(kPad);
        g.setColour(palette_.textPrimary);
        g.setFont(fonts::display(16.0f));
        g.drawText("VOICE & MICROPHONE", c.removeFromTop(24), juce::Justification::centredLeft, false);

        c.removeFromTop(28 + 12); // profile combo
        const char* names[] = {"RNNoise Suppression", "Noise Gate", "Echo Cancellation",
                               "Voice Enhancer", "Side-tone"};
        for (const char* n : names)
        {
            auto r = c.removeFromTop(26);
            g.setColour(palette_.textSecondary);
            g.setFont(fonts::body(13.0f));
            g.drawText(n, r.withTrimmedRight(50), juce::Justification::centredLeft, false);
        }

        c.removeFromTop(6);
        auto meter = c.removeFromTop(16).reduced(0, 5);
        const int segs = 22;
        const float sw = (float) meter.getWidth() / segs;
        for (int i = 0; i < segs; ++i)
        {
            g.setColour(i < 14 ? palette_.success : palette_.strokeHover);
            g.fillRect(juce::Rectangle<float>(meter.getX() + i * sw, (float) meter.getY(),
                                              sw - 2.0f, (float) meter.getHeight()));
        }
    }

private:
    void wire(ToggleSwitch& t, std::function<void(bool)> apply)
    {
        ToggleSwitch* tp = &t;
        auto fn = std::move(apply);
        t.onClick = [this, tp, fn] { fn(tp->getToggleState()); emit(); };
        addAndMakeVisible(t);
    }
    void emit() { if (onChanged) onChanged(cfg_); repaint(); }
    static constexpr int kPad = 20;

    veyra::VoiceConfig cfg_;
    juce::ComboBox profile_;
    ToggleSwitch   rnnoise_, gate_, aec_, enhancer_, sidetone_;
};

// ===========================================================================
// Night Mode
// ===========================================================================
class GamerModeScreen::NightCard : public GlassPanel {
public:
    std::function<void(const veyra::LoudnessConfig&)> onChanged;

    NightCard()
    {
        enable_.onClick = [this]
        {
            const bool on = enable_.getToggleState();
            if (on && cfg_.nightModeAmount <= 0.001f) cfg_.nightModeAmount = 0.6f;
            else if (!on) cfg_.nightModeAmount = 0.0f;
            strength_.setValue(cfg_.nightModeAmount, juce::dontSendNotification);
            emit();
        };
        addAndMakeVisible(enable_);

        styleSlider(strength_);
        strength_.onValueChange = [this]
        {
            cfg_.nightModeAmount = (float) strength_.getValue();
            emit();
        };
        addAndMakeVisible(strength_);
    }

    void setPalette(const Palette& p) override
    {
        GlassPanel::setPalette(p);
        enable_.setPalette(p);
    }

    void setLoudness(const veyra::LoudnessConfig& l)
    {
        cfg_ = l;
        enable_.setToggleState(l.nightModeAmount > 0.001f, juce::dontSendNotification);
        strength_.setValue(l.nightModeAmount, juce::dontSendNotification);
        repaint();
    }

    void resized() override
    {
        auto c = getLocalBounds().reduced(kPad);
        auto title = c.removeFromTop(24);
        enable_.setBounds(title.removeFromRight(46).withSizeKeepingCentre(46, 22));
        c.removeFromTop(34);                  // description (painted)
        c.removeFromTop(16);                  // strength label
        strength_.setBounds(c.removeFromTop(22));
    }

protected:
    void paintContent(juce::Graphics& g) override
    {
        auto c = getLocalBounds().reduced(kPad);
        g.setColour(palette_.textPrimary);
        g.setFont(fonts::display(16.0f));
        g.drawText("NIGHT MODE", c.removeFromTop(24), juce::Justification::centredLeft, false);

        g.setColour(palette_.textTertiary);
        g.setFont(fonts::body(11.0f));
        g.drawFittedText("Reduces dynamic range and deep bass to avoid disturbing others "
                         "while keeping clarity.",
                         c.removeFromTop(34), juce::Justification::topLeft, 2);

        auto sl = c.removeFromTop(16);
        g.setColour(palette_.textSecondary);
        g.setFont(fonts::body(12.0f));
        g.drawText("Strength", sl.removeFromLeft(80), juce::Justification::centredLeft, false);
        g.setColour(palette_.textTertiary);
        g.setFont(fonts::mono(11.0f));
        g.drawText(pct(strength_.getValue()), sl.removeFromRight(48), juce::Justification::centredRight, false);

        c.removeFromTop(22 + 12);
        g.setColour(palette_.textTertiary);
        g.setFont(fonts::mono(10.0f, true));
        g.drawText("AUTO-ENABLE 22:00 - 07:00", c.removeFromTop(16),
                   juce::Justification::centredLeft, false);
    }

private:
    void emit() { if (onChanged) onChanged(cfg_); repaint(); }
    static constexpr int kPad = 20;

    veyra::LoudnessConfig cfg_;
    ToggleSwitch enable_;
    juce::Slider strength_;
};

// ===========================================================================
// Screen
// ===========================================================================
GamerModeScreen::GamerModeScreen()
{
    tracker_ = std::make_unique<TrackerCard>();
    spatial_ = std::make_unique<SpatialCard>();
    voice_   = std::make_unique<VoiceCard>();
    night_   = std::make_unique<NightCard>();

    tracker_->onChanged = [this](const veyra::GamerModeConfig& g)
    {
        master_.setToggleState(g.enabled, juce::dontSendNotification);
        if (onGamerChanged) onGamerChanged(g);
    };
    spatial_->onChanged = [this](const veyra::SpatialConfig& s) { if (onSpatialChanged) onSpatialChanged(s); };
    spatial_->onTest    = [this] { if (onTestInSoundLab) onTestInSoundLab(); };
    voice_->onChanged   = [this](const veyra::VoiceConfig& v) { if (onVoiceChanged) onVoiceChanged(v); };
    night_->onChanged   = [this](const veyra::LoudnessConfig& l) { if (onLoudnessChanged) onLoudnessChanged(l); };

    master_.onClick = [this] { tracker_->setEnabledFromHeader(master_.getToggleState()); };

    addAndMakeVisible(*tracker_);
    addAndMakeVisible(*spatial_);
    addAndMakeVisible(*voice_);
    addAndMakeVisible(*night_);
    addAndMakeVisible(master_);
}

GamerModeScreen::~GamerModeScreen() = default;

void GamerModeScreen::setPalette(const Palette& p)
{
    palette_ = p;
    tracker_->setPalette(p);
    spatial_->setPalette(p);
    voice_->setPalette(p);
    night_->setPalette(p);
    master_.setPalette(p);
    repaint();
}

void GamerModeScreen::attachBackdrop(GlassBackground* b)
{
    tracker_->setBackdrop(b);
    spatial_->setBackdrop(b);
    voice_->setBackdrop(b);
    night_->setBackdrop(b);
}

void GamerModeScreen::setGamer(const veyra::GamerModeConfig& g)
{
    tracker_->setGamer(g);
    master_.setToggleState(g.enabled, juce::dontSendNotification);
}
void GamerModeScreen::setSpatial(const veyra::SpatialConfig& s)  { spatial_->setSpatial(s); }
void GamerModeScreen::setVoice(const veyra::VoiceConfig& v)      { voice_->setVoice(v); }
void GamerModeScreen::setLoudness(const veyra::LoudnessConfig& l) { night_->setLoudness(l); }

void GamerModeScreen::paint(juce::Graphics& g)
{
    auto top = getLocalBounds().reduced(24).removeFromTop(48);
    g.setColour(palette_.textPrimary);
    g.setFont(fonts::display(24.0f));
    g.drawText("GAMER MODE", top.removeFromTop(28), juce::Justification::topLeft, false);
    g.setColour(palette_.textSecondary);
    g.setFont(fonts::body(13.0f));
    g.drawText("Optimized audio for competitive and immersive gaming.", top,
               juce::Justification::topLeft, false);

    g.setColour(palette_.textTertiary);
    g.setFont(fonts::mono(10.0f, true));
    auto mr = getLocalBounds().reduced(24).removeFromTop(28);
    g.drawText("MASTER TOGGLE", mr.removeFromRight(180).withTrimmedRight(54),
               juce::Justification::centredRight, false);
}

void GamerModeScreen::resized()
{
    auto b = getLocalBounds().reduced(24);
    auto header = b.removeFromTop(48);
    master_.setBounds(header.removeFromRight(50).withSizeKeepingCentre(50, 26));
    b.removeFromTop(16);

    const int gap = 16;
    const int colW = (b.getWidth() - gap) / 2;
    const int rowH = (b.getHeight() - gap) / 2;
    auto place = [&](juce::Component& c, int col, int row)
    {
        c.setBounds(b.getX() + col * (colW + gap), b.getY() + row * (rowH + gap), colW, rowH);
    };
    place(*tracker_, 0, 0);
    place(*spatial_, 1, 0);
    place(*voice_,   0, 1);
    place(*night_,   1, 1);
}

} // namespace veyra::ui
