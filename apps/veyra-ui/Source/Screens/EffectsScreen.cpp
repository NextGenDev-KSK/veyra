#include "Screens/EffectsScreen.h"

#include "Components/GlassPanel.h"
#include "Components/IconButton.h"
#include "Graphics/GlassBackground.h"
#include "Graphics/Icons.h"
#include "Theme/Fonts.h"

#include <algorithm>
#include <cmath>
#include <iterator>
#include <vector>

namespace veyra::ui {

namespace {
juce::String dbText(float db)  { return (db >= 0 ? "+" : "") + juce::String(juce::roundToInt(db)) + " dB"; }
juce::String pctText(float fr) { return juce::String(juce::roundToInt(fr * 100.0f)) + "%"; }
} // namespace

class EffectsScreen::RackCard : public GlassPanel {
public:
    RackCard()
    {
        setElevated(true);
        back_.onClick = [this] { if (onBack) onBack(); };
        addAndMakeVisible(back_);
    }

    std::function<void()> onBack;

    void setPalette(const Palette& p) override
    {
        GlassPanel::setPalette(p);
        back_.setPalette(p);
    }

    void setConfig(const veyra::Config& c) { cfg_ = c; repaint(); }

    void resized() override
    {
        back_.setBounds(getLocalBounds().reduced(kPad).removeFromTop(34).removeFromRight(34));
    }

protected:
    void paintContent(juce::Graphics& g) override
    {
        auto content = getLocalBounds().reduced(kPad);

        auto header = content.removeFromTop(34);
        g.setColour(palette_.textPrimary);
        g.setFont(fonts::display(22.0f));
        g.drawText("EFFECTS RACK", header, juce::Justification::centredLeft, false);

        content.removeFromTop(4);
        g.setColour(palette_.textTertiary);
        g.setFont(fonts::body(12.0f));
        g.drawText("The output signal chain, in order. Configure each on Home or in Settings.",
                   content.removeFromTop(20), juce::Justification::centredLeft, false);
        content.removeFromTop(10);

        const auto& e = cfg_.enhancement;
        const bool eqActive = std::any_of(std::begin(e.eqBandsDb), std::end(e.eqBandsDb),
                                          [](float v) { return std::fabs(v) > 0.05f; });

        struct Row { juce::String name, desc, value; bool active; bool coming; };
        const std::vector<Row> rows = {
            {"Equalizer",    "10-band graphic EQ",                "10-band",            eqActive, false},
            {"Bass Boost",   "Low-shelf lift @ 80 Hz",            dbText(e.bassBoostDb), e.bassBoostDb > 0.05f, false},
            {"Treble",       "High-shelf lift @ 8 kHz",           dbText(e.trebleDb),    std::fabs(e.trebleDb) > 0.05f, false},
            {"Compression",  "Stereo-linked dynamics",            pctText(e.compressionAmount), e.compressionAmount > 0.01f, false},
            {"Stereo Width", "Mid/side widening",                 pctText(e.stereoWidth), std::fabs(e.stereoWidth - 1.0f) > 0.01f, false},
            {"Volume Gain",  "Pre-amp / make-up gain",            pctText(e.volumeGain),  std::fabs(e.volumeGain - 1.0f) > 0.01f, false},
            {"Crossfeed",    "Headphone spatialisation",          pctText(cfg_.spatial.crossfeed), cfg_.spatial.enabled && cfg_.spatial.crossfeed > 0.01f, false},
            {"Night Mode",   "Late-night loudness compression",   pctText(cfg_.loudness.nightModeAmount), cfg_.loudness.nightModeAmount > 0.01f, false},
            {"Reverb",       "Room / ambience",                   "Coming soon",        false, true},
            {"Limiter",      "True-peak output protection",       "Always on",          true,  false},
        };

        const int rowH = 44;
        for (const auto& r : rows)
        {
            auto row = content.removeFromTop(rowH);
            content.removeFromTop(8);
            if (row.getHeight() < rowH)
                break;

            g.setColour(palette_.bgGlass.withAlpha(0.5f));
            g.fillRoundedRectangle(row.toFloat(), 10.0f);

            // Status dot.
            auto dot = row.removeFromLeft(34);
            const juce::Colour dc = r.coming ? palette_.textTertiary
                                    : r.active ? palette_.accentPrimary
                                               : juce::Colour(70, 73, 96);
            if (r.active && !r.coming)
                juce::DropShadow(palette_.accentGlow, 8, {}).drawForRectangle(
                    g, juce::Rectangle<int>(dot.getCentreX() - 4, dot.getCentreY() - 4, 8, 8));
            g.setColour(dc);
            g.fillEllipse((float) dot.getCentreX() - 4.0f, (float) dot.getCentreY() - 4.0f, 8.0f, 8.0f);

            auto value = row.removeFromRight(120);
            g.setColour(r.coming ? palette_.textTertiary : (r.active ? palette_.textPrimary : palette_.textSecondary));
            g.setFont(fonts::mono(13.0f, r.active));
            g.drawText(r.value, value.withTrimmedRight(8), juce::Justification::centredRight, false);

            row.removeFromLeft(4);
            auto nameArea = row.removeFromTop(row.getHeight() / 2 + 2);
            g.setColour(palette_.textPrimary);
            g.setFont(fonts::body(14.0f, true));
            g.drawText(r.name, nameArea, juce::Justification::bottomLeft, false);
            g.setColour(palette_.textTertiary);
            g.setFont(fonts::body(11.0f));
            g.drawText(r.desc, row, juce::Justification::topLeft, false);
        }
    }

private:
    static constexpr int kPad = 28;
    veyra::Config cfg_;
    IconButton    back_{icons::close, true};
};

EffectsScreen::EffectsScreen()
{
    card_ = std::make_unique<RackCard>();
    card_->onBack = [this] { if (onBack) onBack(); };
    addAndMakeVisible(*card_);
}

EffectsScreen::~EffectsScreen() = default;

void EffectsScreen::setPalette(const Palette& p)       { card_->setPalette(p); }
void EffectsScreen::attachBackdrop(GlassBackground* b) { card_->setBackdrop(b); }
void EffectsScreen::setConfig(const veyra::Config& c)  { card_->setConfig(c); }
void EffectsScreen::resized()                          { card_->setBounds(getLocalBounds().reduced(24)); }

} // namespace veyra::ui
