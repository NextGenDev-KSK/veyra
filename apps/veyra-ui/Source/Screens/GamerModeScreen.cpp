#include "Screens/GamerModeScreen.h"

#include "Components/GlassPanel.h"
#include "Components/Knob.h"
#include "Components/SegmentedControl.h"
#include "Components/ToggleSwitch.h"
#include "Graphics/GlassBackground.h"
#include "Theme/Fonts.h"

namespace veyra::ui {

class GamerModeScreen::Card : public GlassPanel {
public:
    Card()
    {
        enable_.onClick = [this] { cfg_.enabled = enable_.getToggleState(); emit(); };
        addAndMakeVisible(enable_);

        radar_.setItems({"Competitive", "Rich", "Compass"});
        radar_.onChange = [this](int i) { cfg_.radarMode = i; emit(); };
        addAndMakeVisible(radar_);

        sensitivity_.setLabel("Sensitivity");
        sensitivity_.onChange = [this](double v)
        {
            cfg_.sensitivity = (float) v;
            sensitivity_.setValueText(juce::String(juce::roundToInt(v * 100.0)) + "%");
            emit();
        };
        addAndMakeVisible(sensitivity_);

        auto wireDet = [this](ToggleSwitch& t, bool veyra::GamerModeConfig::* field)
        {
            ToggleSwitch* tp = &t;
            t.onClick = [this, tp, field] { cfg_.*field = tp->getToggleState(); emit(); };
            addAndMakeVisible(t);
        };
        wireDet(foot_,   &GamerModeConfig::footsteps);
        wireDet(guns_,   &GamerModeConfig::gunshots);
        wireDet(voices_, &GamerModeConfig::voices);

        launch_.setButtonText("Launch Overlay");
        launch_.onClick = [] { launchOverlay(); };
        addAndMakeVisible(launch_);
    }

    std::function<void(const veyra::GamerModeConfig&)> onGamerChanged;

    void setPalette(const Palette& p) override
    {
        GlassPanel::setPalette(p);
        enable_.setPalette(p);
        radar_.setPalette(p);
        sensitivity_.setPalette(p);
        foot_.setPalette(p); guns_.setPalette(p); voices_.setPalette(p);
    }

    void setGamer(const veyra::GamerModeConfig& g)
    {
        cfg_ = g;
        enable_.setToggleState(g.enabled, juce::dontSendNotification);
        radar_.setSelectedIndex(g.radarMode, false);
        sensitivity_.setValue(g.sensitivity);
        sensitivity_.setValueText(juce::String(juce::roundToInt(g.sensitivity * 100.0f)) + "%");
        foot_.setToggleState(g.footsteps, juce::dontSendNotification);
        guns_.setToggleState(g.gunshots, juce::dontSendNotification);
        voices_.setToggleState(g.voices, juce::dontSendNotification);
        repaint();
    }

    void resized() override
    {
        auto c = getLocalBounds().reduced(kPad);
        auto header = c.removeFromTop(28);
        enable_.setBounds(header.removeFromRight(46).withSizeKeepingCentre(46, 24));

        c.removeFromTop(34);                 // "Overlay radar style" label (painted)
        radar_.setBounds(c.removeFromTop(34).removeFromLeft(320));

        c.removeFromTop(20);
        auto knobRow = c.removeFromTop(96);
        sensitivity_.setBounds(knobRow.removeFromLeft(96));

        c.removeFromTop(18);                 // "Detect" label (painted)
        auto detRow = c.removeFromTop(28);
        const int sw = 46;
        foot_.setBounds(detRow.getX() + 90, detRow.getY(), sw, 24);
        guns_.setBounds(detRow.getX() + 230, detRow.getY(), sw, 24);
        voices_.setBounds(detRow.getX() + 370, detRow.getY(), sw, 24);

        launch_.setBounds(getLocalBounds().reduced(kPad).removeFromBottom(32).removeFromLeft(150));
    }

protected:
    void paintContent(juce::Graphics& g) override
    {
        auto c = getLocalBounds().reduced(kPad);
        g.setColour(palette_.textPrimary);
        g.setFont(fonts::display(20.0f));
        g.drawText("GAMER MODE", c.removeFromTop(28), juce::Justification::centredLeft, false);

        g.setColour(palette_.textSecondary);
        g.setFont(fonts::body(13.0f));
        g.drawText("Overlay radar style", c.removeFromTop(34).withTrimmedTop(12),
                   juce::Justification::topLeft, false);

        c.removeFromTop(34 + 20 + 96 + 18 - 12); // skip past radar + knob to the detect row
        auto detRow = c.removeFromTop(28);
        g.setColour(palette_.textSecondary);
        g.drawText("Detect", detRow, juce::Justification::centredLeft, false);
        auto label = [&](int x, const char* t)
        {
            g.setColour(palette_.textTertiary);
            g.setFont(fonts::body(12.0f));
            g.drawText(t, juce::Rectangle<int>(getLocalBounds().reduced(kPad).getX() + x, detRow.getY(), 80, 24),
                       juce::Justification::centredLeft, false);
        };
        label(140, "Footsteps");
        label(280, "Gunshots");
        label(420, "Voices");
    }

private:
    static void launchOverlay()
    {
        auto exe = juce::File::getSpecialLocation(juce::File::currentExecutableFile)
                       .getParentDirectory().getChildFile("veyra-overlay.exe");
        if (exe.existsAsFile())
            exe.startAsProcess();
    }

    void emit() { if (onGamerChanged) onGamerChanged(cfg_); repaint(); }

    static constexpr int kPad = 24;

    veyra::GamerModeConfig cfg_;
    ToggleSwitch     enable_, foot_, guns_, voices_;
    SegmentedControl radar_;
    Knob             sensitivity_;
    juce::TextButton launch_;
};

// ---------------------------------------------------------------------------

GamerModeScreen::GamerModeScreen()
{
    card_ = std::make_unique<Card>();
    card_->onGamerChanged = [this](const veyra::GamerModeConfig& g) { if (onGamerChanged) onGamerChanged(g); };
    addAndMakeVisible(*card_);
}

GamerModeScreen::~GamerModeScreen() = default;

void GamerModeScreen::setPalette(const Palette& p)        { card_->setPalette(p); }
void GamerModeScreen::attachBackdrop(GlassBackground* b)  { card_->setBackdrop(b); }
void GamerModeScreen::setGamer(const veyra::GamerModeConfig& g) { card_->setGamer(g); }
void GamerModeScreen::resized() { card_->setBounds(getLocalBounds().reduced(24)); }

} // namespace veyra::ui
