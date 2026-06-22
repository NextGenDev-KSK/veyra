#include "Screens/SettingsScreen.h"

#include "Components/GlassPanel.h"
#include "Components/SegmentedControl.h"
#include "Components/ToggleSwitch.h"
#include "Graphics/GlassBackground.h"
#include "Theme/Fonts.h"

#include "veyra/Paths.h"
#include "veyra/version.h"

#include <cmath>
#include <vector>

namespace veyra::ui {

// ---------------------------------------------------------------------------
// AppearanceCard: the glass card holding the theme grid + appearance controls.
// ---------------------------------------------------------------------------
class SettingsScreen::AppearanceCard : public GlassPanel {
public:
    AppearanceCard()
    {
        themes_ = builtInThemes();

        bgMode_.setItems({"Ambient", "Solid", "Image"});
        bgMode_.setSelectedIndex(0, false);
        bgMode_.onChange = [this](int i) { if (onBackgroundMode) onBackgroundMode(i); };
        addAndMakeVisible(bgMode_);

        opacity_.setSliderStyle(juce::Slider::LinearHorizontal);
        opacity_.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
        opacity_.setRange(0.30, 1.0, 0.01);
        opacity_.setValue(0.85, juce::dontSendNotification);
        opacity_.onValueChange = [this] { repaint(); if (onOpacity) onOpacity(opacity_.getValue()); };
        addAndMakeVisible(opacity_);

        reduceMotion_.onClick = [this] { if (onReduceMotion) onReduceMotion(reduceMotion_.getToggleState()); };
        addAndMakeVisible(reduceMotion_);
    }

    std::function<void(const juce::String&)> onThemeSelected;
    std::function<void(double)>              onOpacity;
    std::function<void(int)>                 onBackgroundMode;
    std::function<void(bool)>                onReduceMotion;

    void setPalette(const Palette& p) override
    {
        GlassPanel::setPalette(p);
        bgMode_.setPalette(p);
        reduceMotion_.setPalette(p);
    }

    void setCurrentTheme(const juce::String& id) { current_ = id; repaint(); }

    void setAppearance(double opacity, int backgroundMode, bool reduceMotion)
    {
        opacity_.setValue(opacity, juce::dontSendNotification);
        bgMode_.setSelectedIndex(backgroundMode, false);
        reduceMotion_.setToggleState(reduceMotion, juce::dontSendNotification);
        repaint();
    }

    void resized() override
    {
        const Layout l = layout();
        opacity_.setBounds(l.opacityCtl);
        bgMode_.setBounds(l.bgCtl);
        reduceMotion_.setBounds(l.reduceCtl);
    }

    void mouseMove(const juce::MouseEvent& e) override
    {
        const int h = hitTest(e.getPosition());
        if (h != hover_) { hover_ = h; repaint(); }
    }

    void mouseExit(const juce::MouseEvent&) override
    {
        if (hover_ != -1) { hover_ = -1; repaint(); }
    }

    void mouseDown(const juce::MouseEvent& e) override
    {
        const int h = hitTest(e.getPosition());
        if (h >= 0)
        {
            current_ = themes_[(size_t) h].id;
            repaint();
            if (onThemeSelected)
                onThemeSelected(current_);
        }
    }

protected:
    void paintContent(juce::Graphics& g) override
    {
        const Layout l = layout();
        auto content = getLocalBounds().reduced(kPad);

        // Card title.
        g.setColour(palette_.textPrimary);
        g.setFont(fonts::display(20.0f));
        g.drawText("APPEARANCE", content.removeFromTop(28), juce::Justification::centredLeft, false);

        // "Theme" section label.
        g.setColour(palette_.textSecondary);
        g.setFont(fonts::body(12.0f, true));
        g.drawText("THEME", juce::Rectangle<int>(l.grid.getX(), l.grid.getY() - 22, 200, 16),
                   juce::Justification::centredLeft, false);

        // Theme preview cells.
        for (int i = 0; i < (int) themes_.size(); ++i)
        {
            const auto cb = cellBounds(i, l.grid).toFloat();
            const Palette tp = paletteForTheme(themes_[(size_t) i].id);
            const bool selected = themes_[(size_t) i].id == current_;
            const bool hover = (i == hover_);

            // Mini "app" preview: canvas fill + a few accent bars.
            g.setColour(tp.bgApp);
            g.fillRoundedRectangle(cb, 10.0f);

            auto inner = cb.reduced(8.0f);
            auto preview = inner.removeFromTop(inner.getHeight() - 18.0f);
            g.setColour(tp.bgGlass);
            g.fillRoundedRectangle(preview, 6.0f);

            const float bx = preview.getX() + 6.0f;
            const float bb = preview.getBottom() - 6.0f;
            const float bh[] = {10.0f, 18.0f, 13.0f, 22.0f, 8.0f};
            for (int b = 0; b < 5; ++b)
            {
                g.setColour(b % 2 ? tp.accentSecondary : tp.accentPrimary);
                g.fillRoundedRectangle(bx + b * 7.0f, bb - bh[b], 4.0f, bh[b], 2.0f);
            }

            // Name (in the theme's own text colour so contrast is visible).
            g.setColour(tp.textPrimary);
            g.setFont(fonts::body(11.0f, selected));
            g.drawText(themes_[(size_t) i].name, inner.toNearestInt(),
                       juce::Justification::centredLeft, false);

            // Selection ring / hover stroke.
            if (selected)
            {
                juce::DropShadow(palette_.accentGlow, 10, {}).drawForRectangle(g, cb.toNearestInt());
                g.setColour(palette_.accentPrimary);
                g.drawRoundedRectangle(cb.reduced(1.0f), 10.0f, 2.0f);
            }
            else
            {
                g.setColour(hover ? palette_.strokeHover : palette_.strokeDefault);
                g.drawRoundedRectangle(cb.reduced(0.5f), 10.0f, 1.0f);
            }
        }

        // Control rows: label on the left, control on the right.
        auto rowLabel = [&](juce::Rectangle<int> row, const char* title, const char* sub)
        {
            g.setColour(palette_.textPrimary);
            g.setFont(fonts::body(14.0f, true));
            g.drawText(title, row.removeFromTop(20), juce::Justification::topLeft, false);
            g.setColour(palette_.textTertiary);
            g.setFont(fonts::body(11.0f));
            g.drawText(sub, row, juce::Justification::topLeft, false);
        };
        rowLabel(l.opacityRow, "UI Opacity", "Glass translucency across panels");
        rowLabel(l.bgRow, "Background", "Ambient blobs, solid fill, or an image");
        rowLabel(l.reduceRow, "Reduce Motion", "Freeze the visualizer and transitions");

        // Opacity percentage readout.
        g.setColour(palette_.textSecondary);
        g.setFont(fonts::mono(12.0f));
        g.drawText(juce::String(juce::roundToInt(opacity_.getValue() * 100.0)) + "%",
                   juce::Rectangle<int>(l.opacityCtl.getRight() + 8, l.opacityCtl.getY() - 4,
                                        44, l.opacityCtl.getHeight() + 8),
                   juce::Justification::centredLeft, false);
    }

private:
    struct Layout {
        juce::Rectangle<int> grid;
        juce::Rectangle<int> opacityRow, opacityCtl;
        juce::Rectangle<int> bgRow, bgCtl;
        juce::Rectangle<int> reduceRow, reduceCtl;
    };

    Layout layout() const
    {
        auto content = getLocalBounds().reduced(kPad);
        Layout l;
        content.removeFromTop(28);     // title
        content.removeFromTop(22 + 6); // "THEME" label + gap
        l.grid = content.removeFromTop(kRows * kCellH + (kRows - 1) * kGap);
        content.removeFromTop(20);     // divider gap

        const int ctlW = juce::jmin(300, content.getWidth() / 2);
        auto take = [&](juce::Rectangle<int>& ctl, int ctlH) -> juce::Rectangle<int>
        {
            auto row = content.removeFromTop(48);
            content.removeFromTop(8);
            ctl = juce::Rectangle<int>(row.getRight() - ctlW - 52, row.getCentreY() - ctlH / 2, ctlW, ctlH);
            return row;
        };
        l.opacityRow = take(l.opacityCtl, 20);
        l.bgRow      = take(l.bgCtl, 34);
        l.reduceRow  = take(l.reduceCtl, 20);
        // Reduce-motion uses a compact toggle pinned to the right edge.
        l.reduceCtl = juce::Rectangle<int>(l.reduceRow.getRight() - 44, l.reduceRow.getCentreY() - 10, 44, 20);
        return l;
    }

    juce::Rectangle<int> cellBounds(int i, juce::Rectangle<int> grid) const
    {
        const int col = i % kCols, row = i / kCols;
        const int cw = (grid.getWidth() - kGap * (kCols - 1)) / kCols;
        return {grid.getX() + col * (cw + kGap), grid.getY() + row * (kCellH + kGap), cw, kCellH};
    }

    int hitTest(juce::Point<int> p) const
    {
        const auto grid = layout().grid;
        for (int i = 0; i < (int) themes_.size(); ++i)
            if (cellBounds(i, grid).contains(p))
                return i;
        return -1;
    }

    static constexpr int kPad = 28;
    static constexpr int kCols = 4;
    static constexpr int kRows = 3; // ceil(11 / 4)
    static constexpr int kCellH = 70;
    static constexpr int kGap = 12;

    std::vector<ThemeInfo> themes_;
    SegmentedControl       bgMode_;
    ToggleSwitch           reduceMotion_;
    juce::Slider           opacity_;
    juce::String           current_{"midnight"};
    int                    hover_ = -1;
};

// ---------------------------------------------------------------------------
// MicrophoneCard: the voice-chain controls (Phase 6).
// ---------------------------------------------------------------------------
class SettingsScreen::MicrophoneCard : public GlassPanel {
public:
    MicrophoneCard()
    {
        enable_.setToggleState(true, juce::dontSendNotification);
        enable_.onClick = [this] { voice_.enabled = enable_.getToggleState(); emit(); };
        addAndMakeVisible(enable_);

        auto add = [this](juce::Slider& s, double lo, double hi, double def)
        {
            s.setSliderStyle(juce::Slider::LinearHorizontal);
            s.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
            s.setRange(lo, hi, 0.01);
            s.setValue(def, juce::dontSendNotification);
            s.onValueChange = [this] { pull(); emit(); };
            addAndMakeVisible(s);
        };
        add(ns_, 0.0, 1.0, 0.5);
        add(comp_, 0.0, 1.0, 0.3);
        add(deess_, 0.0, 1.0, 0.3);
        add(presence_, -6.0, 9.0, 2.0);
        add(gain_, -12.0, 12.0, 0.0);
        add(side_, 0.0, 1.0, 0.0);
    }

    std::function<void(const veyra::VoiceConfig&)> onMicChanged;

    void setPalette(const Palette& p) override
    {
        GlassPanel::setPalette(p);
        enable_.setPalette(p);
    }

    void setMicConfig(const veyra::VoiceConfig& v)
    {
        voice_ = v;
        enable_.setToggleState(v.enabled, juce::dontSendNotification);
        ns_.setValue(v.noiseSuppression, juce::dontSendNotification);
        comp_.setValue(v.compressionAmount, juce::dontSendNotification);
        deess_.setValue(v.deEssAmount, juce::dontSendNotification);
        presence_.setValue(v.presenceDb, juce::dontSendNotification);
        gain_.setValue(v.outputGainDb, juce::dontSendNotification);
        side_.setValue(v.sideToneLevel, juce::dontSendNotification);
        repaint();
    }

    void resized() override
    {
        auto header = getLocalBounds().reduced(kPad).removeFromTop(28);
        enable_.setBounds(header.removeFromRight(40).withSizeKeepingCentre(40, 20));

        juce::Slider* sliders[] = {&ns_, &comp_, &deess_, &presence_, &gain_, &side_};
        for (int i = 0; i < 6; ++i)
        {
            auto row = rowArea(i);
            row.removeFromLeft(150);
            row.removeFromRight(56);
            sliders[i]->setBounds(row.withSizeKeepingCentre(row.getWidth(), 18));
        }
    }

protected:
    void paintContent(juce::Graphics& g) override
    {
        g.setColour(palette_.textPrimary);
        g.setFont(fonts::display(20.0f));
        g.drawText("MICROPHONE", getLocalBounds().reduced(kPad).removeFromTop(28),
                   juce::Justification::centredLeft, false);

        struct Row { const char* label; juce::Slider* s; bool db; };
        Row rows[] = {
            {"Noise Suppression", &ns_, false}, {"Compression", &comp_, false},
            {"De-ess", &deess_, false},          {"Presence", &presence_, true},
            {"Output Gain", &gain_, true},       {"Side-tone", &side_, false},
        };
        for (int i = 0; i < 6; ++i)
        {
            auto row = rowArea(i);
            g.setColour(enable_.getToggleState() ? palette_.textSecondary : palette_.textTertiary);
            g.setFont(fonts::body(13.0f));
            g.drawText(rows[i].label, row.removeFromLeft(150).withTrimmedRight(8),
                       juce::Justification::centredLeft, false);

            const double v = rows[i].s->getValue();
            const juce::String val = rows[i].db
                ? (v >= 0 ? "+" : "") + juce::String(juce::roundToInt(v)) + " dB"
                : juce::String(juce::roundToInt(v * 100.0)) + "%";
            g.setColour(palette_.textPrimary);
            g.setFont(fonts::mono(12.0f));
            g.drawText(val, row.removeFromRight(52), juce::Justification::centredRight, false);
        }
    }

private:
    juce::Rectangle<int> rowArea(int i) const
    {
        auto c = getLocalBounds().reduced(kPad);
        c.removeFromTop(kHeaderH);
        return {c.getX(), c.getY() + i * (kRowH + kRowGap), c.getWidth(), kRowH};
    }

    void pull()
    {
        voice_.noiseSuppression  = (float) ns_.getValue();
        voice_.compressionAmount = (float) comp_.getValue();
        voice_.deEssAmount       = (float) deess_.getValue();
        voice_.presenceDb        = (float) presence_.getValue();
        voice_.outputGainDb      = (float) gain_.getValue();
        voice_.sideToneLevel     = (float) side_.getValue();
    }

    void emit()
    {
        if (onMicChanged)
            onMicChanged(voice_);
        repaint();
    }

    static constexpr int kPad = 24, kHeaderH = 44, kRowH = 36, kRowGap = 10;

    veyra::VoiceConfig voice_;
    ToggleSwitch       enable_;
    juce::Slider       ns_, comp_, deess_, presence_, gain_, side_;
};

// ---------------------------------------------------------------------------
// SpatialCard: headphone spatial controls (Phase 7).
// ---------------------------------------------------------------------------
class SettingsScreen::SpatialCard : public GlassPanel {
public:
    SpatialCard()
    {
        enable_.setToggleState(false, juce::dontSendNotification);
        enable_.onClick = [this] { spatial_.enabled = enable_.getToggleState(); emit(); };
        addAndMakeVisible(enable_);

        mode_.setItems({"Off", "Cinematic", "Competitive"});
        mode_.setSelectedIndex(0, false);
        mode_.onChange = [this](int i) { applyMode(i); };
        addAndMakeVisible(mode_);

        cf_.setSliderStyle(juce::Slider::LinearHorizontal);
        cf_.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
        cf_.setRange(0.0, 1.0, 0.01);
        cf_.setValue(0.0, juce::dontSendNotification);
        cf_.onValueChange = [this] { spatial_.crossfeed = (float) cf_.getValue(); emit(); };
        addAndMakeVisible(cf_);

        vs_.setSliderStyle(juce::Slider::LinearHorizontal);
        vs_.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
        vs_.setRange(0.0, 1.0, 0.01);
        vs_.setValue(0.0, juce::dontSendNotification);
        vs_.onValueChange = [this] { spatial_.virtualization = (float) vs_.getValue(); emit(); };
        addAndMakeVisible(vs_);
    }

    std::function<void(const veyra::SpatialConfig&)> onSpatialChanged;

    void setPalette(const Palette& p) override
    {
        GlassPanel::setPalette(p);
        enable_.setPalette(p);
        mode_.setPalette(p);
    }

    void setSpatialConfig(const veyra::SpatialConfig& s)
    {
        spatial_ = s;
        enable_.setToggleState(s.enabled, juce::dontSendNotification);
        mode_.setSelectedIndex(juce::jlimit(0, 2, s.mode), false);
        cf_.setValue(s.crossfeed, juce::dontSendNotification);
        vs_.setValue(s.virtualization, juce::dontSendNotification);
        repaint();
    }

    void resized() override
    {
        auto header = getLocalBounds().reduced(kPad).removeFromTop(28);
        enable_.setBounds(header.removeFromRight(40).withSizeKeepingCentre(40, 20));
        mode_.setBounds(modeRow());
        auto row = crossfeedRow();
        row.removeFromLeft(110);
        row.removeFromRight(52);
        cf_.setBounds(row.withSizeKeepingCentre(row.getWidth(), 18));
        auto vrow = virtualizationRow();
        vrow.removeFromLeft(110);
        vrow.removeFromRight(52);
        vs_.setBounds(vrow.withSizeKeepingCentre(vrow.getWidth(), 18));
    }

protected:
    void paintContent(juce::Graphics& g) override
    {
        g.setColour(palette_.textPrimary);
        g.setFont(fonts::display(20.0f));
        g.drawText("SPATIAL", getLocalBounds().reduced(kPad).removeFromTop(28),
                   juce::Justification::centredLeft, false);

        auto row = crossfeedRow();
        g.setColour(spatial_.enabled ? palette_.textSecondary : palette_.textTertiary);
        g.setFont(fonts::body(13.0f));
        g.drawText("Crossfeed", row.removeFromLeft(110).withTrimmedRight(8),
                   juce::Justification::centredLeft, false);
        g.setColour(palette_.textPrimary);
        g.setFont(fonts::mono(12.0f));
        g.drawText(juce::String(juce::roundToInt(cf_.getValue() * 100.0)) + "%",
                   row.removeFromRight(52), juce::Justification::centredRight, false);

        auto vrow = virtualizationRow();
        g.setColour(spatial_.enabled ? palette_.textSecondary : palette_.textTertiary);
        g.setFont(fonts::body(13.0f));
        g.drawText("Virtualization", vrow.removeFromLeft(110).withTrimmedRight(8),
                   juce::Justification::centredLeft, false);
        g.setColour(palette_.textPrimary);
        g.setFont(fonts::mono(12.0f));
        g.drawText(juce::String(juce::roundToInt(vs_.getValue() * 100.0)) + "%",
                   vrow.removeFromRight(52), juce::Justification::centredRight, false);
    }

private:
    juce::Rectangle<int> modeRow() const
    {
        auto c = getLocalBounds().reduced(kPad);
        c.removeFromTop(28 + 8);
        return c.removeFromTop(34);
    }
    juce::Rectangle<int> crossfeedRow() const
    {
        auto c = getLocalBounds().reduced(kPad);
        c.removeFromTop(28 + 8 + 34 + 12);
        return c.removeFromTop(36);
    }
    juce::Rectangle<int> virtualizationRow() const
    {
        auto c = getLocalBounds().reduced(kPad);
        c.removeFromTop(28 + 8 + 34 + 12 + 36 + 10);
        return c.removeFromTop(36);
    }

    void applyMode(int i)
    {
        spatial_.mode = i;
        if (i == 1)      { spatial_.enabled = true;  spatial_.crossfeed = 0.55f; spatial_.virtualization = 0.70f; } // Cinematic
        else if (i == 2) { spatial_.enabled = true;  spatial_.crossfeed = 0.25f; spatial_.virtualization = 0.0f;  } // Competitive
        else             { spatial_.enabled = false; spatial_.crossfeed = 0.0f;  spatial_.virtualization = 0.0f;  } // Off
        enable_.setToggleState(spatial_.enabled, juce::dontSendNotification);
        cf_.setValue(spatial_.crossfeed, juce::dontSendNotification);
        vs_.setValue(spatial_.virtualization, juce::dontSendNotification);
        emit();
    }

    void emit()
    {
        if (onSpatialChanged)
            onSpatialChanged(spatial_);
        repaint();
    }

    static constexpr int kPad = 24;

    veyra::SpatialConfig spatial_;
    ToggleSwitch         enable_;
    SegmentedControl     mode_;
    juce::Slider         cf_;
    juce::Slider         vs_;
};

// ---------------------------------------------------------------------------
// LoudnessCard: Night Mode (dynamic-range compression) + Sleep Timer.
// ---------------------------------------------------------------------------
class SettingsScreen::LoudnessCard : public GlassPanel {
public:
    LoudnessCard()
    {
        night_.setSliderStyle(juce::Slider::LinearHorizontal);
        night_.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
        night_.setRange(0.0, 1.0, 0.01);
        night_.onValueChange = [this] { loud_.nightModeAmount = (float) night_.getValue(); emit(); };
        addAndMakeVisible(night_);

        sleepEnable_.onClick = [this] { loud_.sleepTimerEnabled = sleepEnable_.getToggleState(); emit(); };
        addAndMakeVisible(sleepEnable_);

        minutes_.setSliderStyle(juce::Slider::LinearHorizontal);
        minutes_.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
        minutes_.setRange(5.0, 120.0, 1.0);
        minutes_.setValue(30.0, juce::dontSendNotification);
        minutes_.onValueChange = [this] { loud_.sleepTimerMinutes = (float) minutes_.getValue(); emit(); };
        addAndMakeVisible(minutes_);

        matchEnable_.onClick = [this] { loud_.loudnessMatch = matchEnable_.getToggleState(); emit(); };
        addAndMakeVisible(matchEnable_);

        eqLoud_.onClick = [this] { loud_.equalLoudness = eqLoud_.getToggleState(); emit(); };
        addAndMakeVisible(eqLoud_);
    }

    std::function<void(const veyra::LoudnessConfig&)> onLoudnessChanged;

    void setPalette(const Palette& p) override
    {
        GlassPanel::setPalette(p);
        sleepEnable_.setPalette(p);
        matchEnable_.setPalette(p);
        eqLoud_.setPalette(p);
    }

    void setLoudnessConfig(const veyra::LoudnessConfig& l)
    {
        loud_ = l;
        night_.setValue(l.nightModeAmount, juce::dontSendNotification);
        sleepEnable_.setToggleState(l.sleepTimerEnabled, juce::dontSendNotification);
        minutes_.setValue(l.sleepTimerMinutes, juce::dontSendNotification);
        matchEnable_.setToggleState(l.loudnessMatch, juce::dontSendNotification);
        eqLoud_.setToggleState(l.equalLoudness, juce::dontSendNotification);
        repaint();
    }

    void resized() override
    {
        auto header = getLocalBounds().reduced(kPad).removeFromTop(28);
        juce::ignoreUnused(header);
        auto nr = nightRow();
        nr.removeFromLeft(110);
        nr.removeFromRight(52);
        night_.setBounds(nr.withSizeKeepingCentre(nr.getWidth(), 18));

        auto sr = sleepRow();
        sleepEnable_.setBounds(sr.removeFromRight(40).withSizeKeepingCentre(40, 20));

        auto mr = minutesRow();
        mr.removeFromLeft(110);
        mr.removeFromRight(64);
        minutes_.setBounds(mr.withSizeKeepingCentre(mr.getWidth(), 18));

        auto mt = matchRow();
        matchEnable_.setBounds(mt.removeFromRight(40).withSizeKeepingCentre(40, 20));

        auto er = eqLoudRow();
        eqLoud_.setBounds(er.removeFromRight(40).withSizeKeepingCentre(40, 20));
    }

protected:
    void paintContent(juce::Graphics& g) override
    {
        g.setColour(palette_.textPrimary);
        g.setFont(fonts::display(20.0f));
        g.drawText("LOUDNESS", getLocalBounds().reduced(kPad).removeFromTop(28),
                   juce::Justification::centredLeft, false);

        // Night Mode row.
        auto nr = nightRow();
        g.setColour(palette_.textSecondary);
        g.setFont(fonts::body(13.0f));
        g.drawText("Night Mode", nr.removeFromLeft(110).withTrimmedRight(8),
                   juce::Justification::centredLeft, false);
        g.setColour(palette_.textPrimary);
        g.setFont(fonts::mono(12.0f));
        g.drawText(juce::String(juce::roundToInt(night_.getValue() * 100.0)) + "%",
                   nr.removeFromRight(52), juce::Justification::centredRight, false);

        // Sleep Timer label.
        auto sr = sleepRow();
        g.setColour(palette_.textPrimary);
        g.setFont(fonts::body(14.0f, true));
        g.drawText("Sleep Timer", sr.removeFromLeft(160), juce::Justification::centredLeft, false);

        // Minutes row.
        auto mr = minutesRow();
        g.setColour(loud_.sleepTimerEnabled ? palette_.textSecondary : palette_.textTertiary);
        g.setFont(fonts::body(13.0f));
        g.drawText("Fade after", mr.removeFromLeft(110).withTrimmedRight(8),
                   juce::Justification::centredLeft, false);
        g.setColour(palette_.textPrimary);
        g.setFont(fonts::mono(12.0f));
        g.drawText(juce::String(juce::roundToInt(minutes_.getValue())) + " min",
                   mr.removeFromRight(64), juce::Justification::centredRight, false);

        // Loudness Match row.
        auto mt = matchRow();
        g.setColour(palette_.textPrimary);
        g.setFont(fonts::body(14.0f, true));
        g.drawText("Loudness Match", mt.removeFromTop(20), juce::Justification::topLeft, false);
        g.setColour(palette_.textTertiary);
        g.setFont(fonts::body(11.0f));
        g.drawText("Auto-match to " + juce::String(juce::roundToInt(loud_.targetLufs)) + " LUFS",
                   mt, juce::Justification::topLeft, false);

        // Equal Loudness row.
        auto er = eqLoudRow();
        g.setColour(palette_.textPrimary);
        g.setFont(fonts::body(14.0f, true));
        g.drawText("Equal Loudness", er.removeFromTop(20), juce::Justification::topLeft, false);
        g.setColour(palette_.textTertiary);
        g.setFont(fonts::body(11.0f));
        g.drawText("Natural bass + treble at low volume", er, juce::Justification::topLeft, false);
    }

private:
    juce::Rectangle<int> nightRow() const
    {
        auto c = getLocalBounds().reduced(kPad);
        c.removeFromTop(28 + 10);
        return c.removeFromTop(36);
    }
    juce::Rectangle<int> sleepRow() const
    {
        auto c = getLocalBounds().reduced(kPad);
        c.removeFromTop(28 + 10 + 36 + 14);
        return c.removeFromTop(24);
    }
    juce::Rectangle<int> minutesRow() const
    {
        auto c = getLocalBounds().reduced(kPad);
        c.removeFromTop(28 + 10 + 36 + 14 + 24 + 6);
        return c.removeFromTop(36);
    }
    juce::Rectangle<int> matchRow() const
    {
        auto c = getLocalBounds().reduced(kPad);
        c.removeFromTop(28 + 10 + 36 + 14 + 24 + 6 + 36 + 10);
        return c.removeFromTop(36);
    }
    juce::Rectangle<int> eqLoudRow() const
    {
        auto c = getLocalBounds().reduced(kPad);
        c.removeFromTop(28 + 10 + 36 + 14 + 24 + 6 + 36 + 10 + 36 + 10);
        return c.removeFromTop(36);
    }

    void emit()
    {
        if (onLoudnessChanged)
            onLoudnessChanged(loud_);
        repaint();
    }

    static constexpr int kPad = 24;

    veyra::LoudnessConfig loud_;
    juce::Slider          night_;
    ToggleSwitch          sleepEnable_;
    juce::Slider          minutes_;
    ToggleSwitch          matchEnable_;
    ToggleSwitch          eqLoud_;
};

// ---------------------------------------------------------------------------
// AboutCard: build info, service status, licenses + maintenance actions.
// ---------------------------------------------------------------------------
class SettingsScreen::AboutCard : public GlassPanel {
public:
    AboutCard()
    {
        openLogs_.setButtonText("Open Logs");
        openLogs_.onClick = [] {
            const juce::File dir(juce::String(veyra::paths::logsDir().wstring().c_str()));
            dir.createDirectory();
            dir.revealToUser();
        };
        addAndMakeVisible(openLogs_);

        reset_.setButtonText("Reset Settings");
        reset_.onClick = [this] { if (onResetSettings) onResetSettings(); };
        addAndMakeVisible(reset_);
    }

    std::function<void()> onResetSettings;

    void setPalette(const Palette& p) override { GlassPanel::setPalette(p); repaint(); }

    void setServiceStatus(bool connected, juce::String version)
    {
        connected_ = connected;
        version_ = std::move(version);
        repaint();
    }

    void resized() override
    {
        auto b = getLocalBounds().reduced(kPad);
        auto buttons = b.removeFromRight(160);
        reset_.setBounds(buttons.removeFromBottom(34));
        buttons.removeFromBottom(10);
        openLogs_.setBounds(buttons.removeFromBottom(34));
    }

protected:
    void paintContent(juce::Graphics& g) override
    {
        auto b = getLocalBounds().reduced(kPad);

        g.setColour(palette_.textPrimary);
        g.setFont(fonts::display(20.0f));
        g.drawText("ABOUT", b.removeFromTop(28), juce::Justification::centredLeft, false);

        auto line = [&](const juce::String& label, const juce::String& value, juce::Colour vc)
        {
            auto row = b.removeFromTop(20);
            g.setColour(palette_.textTertiary);
            g.setFont(fonts::body(12.0f));
            g.drawText(label, row.removeFromLeft(130), juce::Justification::centredLeft, false);
            g.setColour(vc);
            g.setFont(fonts::mono(12.0f));
            g.drawText(value, row, juce::Justification::centredLeft, false);
        };

        line("Version", juce::String("Veyra Sounds ") + veyra::kVersionString, palette_.textPrimary);
        line("Build", juce::String(veyra::kGitCommit), palette_.textSecondary);
        line("Audio engine", juce::String("v") + veyra::kVersionString, palette_.textSecondary);
        line("Service",
             connected_ ? juce::String("connected (") + version_ + ")" : juce::String("disconnected"),
             connected_ ? palette_.success : palette_.warning);

        b.removeFromTop(8);
        g.setColour(palette_.textTertiary);
        g.setFont(fonts::body(11.0f));
        g.drawText(juce::String::fromUTF8("\xc2\xa9 2026 Krithik S \xc2\xb7 GPLv3 \xc2\xb7 NextGenDev-KSK"),
                   b.removeFromTop(16), juce::Justification::centredLeft, false);
        g.drawText("JUCE  \xc2\xb7  spdlog  \xc2\xb7  nlohmann/json  \xc2\xb7  Catch2  \xc2\xb7  Orbitron/Inter/JetBrains Mono (OFL)",
                   b.removeFromTop(16), juce::Justification::centredLeft, false);
    }

private:
    static constexpr int kPad = 24;
    juce::TextButton openLogs_, reset_;
    bool         connected_ = false;
    juce::String version_;
};

// ---------------------------------------------------------------------------
// SettingsScreen: thin host that forwards to the cards.
// ---------------------------------------------------------------------------
// ===========================================================================
// Audio Engine
// ===========================================================================
class SettingsScreen::AudioEngineCard : public GlassPanel {
public:
    std::function<void(const veyra::AudioEngineConfig&)> onChanged;
    std::function<void(bool)> onReferenceModeChanged;

    AudioEngineCard()
    {
        hwAccel_.onClick = [this] { cfg_.hardwareAcceleration = hwAccel_.getToggleState(); emit(); };
        addAndMakeVisible(hwAccel_);
        reference_.onClick = [this]
        { if (onReferenceModeChanged) onReferenceModeChanged(reference_.getToggleState()); };
        addAndMakeVisible(reference_);
        lowLatency_.onClick = [this]
        { cfg_.latencyMode = lowLatency_.getToggleState() ? "UltraLow" : "Standard"; emit(); };
        addAndMakeVisible(lowLatency_);

        rate_.setItems({"44.1 kHz", "48 kHz", "96 kHz", "192 kHz"});
        rate_.onChange = [this](int i) { cfg_.sampleRate = kRates[(size_t) juce::jlimit(0, 3, i)]; emit(); };
        addAndMakeVisible(rate_);

        buffer_.setSliderStyle(juce::Slider::LinearHorizontal);
        buffer_.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
        buffer_.setRange(64.0, 2048.0, 1.0);
        buffer_.onValueChange = [this] { cfg_.bufferSize = (int) buffer_.getValue(); repaint(); emit(); };
        addAndMakeVisible(buffer_);
    }

    void setPalette(const Palette& p) override
    {
        GlassPanel::setPalette(p);
        hwAccel_.setPalette(p);
        lowLatency_.setPalette(p);
        reference_.setPalette(p);
        rate_.setPalette(p);
    }

    void setReferenceMode(bool on)
    {
        reference_.setToggleState(on, juce::dontSendNotification);
        repaint();
    }

    void setConfig(const veyra::AudioEngineConfig& e)
    {
        cfg_ = e;
        hwAccel_.setToggleState(e.hardwareAcceleration, juce::dontSendNotification);
        lowLatency_.setToggleState(e.latencyMode == "UltraLow", juce::dontSendNotification);
        int idx = 1;
        for (int i = 0; i < 4; ++i) if (kRates[i] == e.sampleRate) idx = i;
        rate_.setSelectedIndex(idx, false);
        buffer_.setValue(e.bufferSize, juce::dontSendNotification);
        repaint();
    }

    void resized() override
    {
        auto c = getLocalBounds().reduced(kPad);
        c.removeFromTop(28);                 // title
        c.removeFromTop(18);                 // subtitle
        auto t1 = c.removeFromTop(28);
        hwAccel_.setBounds(t1.removeFromRight(46).withSizeKeepingCentre(46, 22));
        c.removeFromTop(10);
        auto t2 = c.removeFromTop(28);
        lowLatency_.setBounds(t2.removeFromRight(46).withSizeKeepingCentre(46, 22));
        c.removeFromTop(18);
        c.removeFromTop(18);                 // "Sample Rate" label
        rate_.setBounds(c.removeFromTop(32));
        c.removeFromTop(18);
        c.removeFromTop(18);                 // "Buffer Size" label
        buffer_.setBounds(c.removeFromTop(24));
        c.removeFromTop(16);
        auto tr = c.removeFromTop(28);
        reference_.setBounds(tr.removeFromRight(46).withSizeKeepingCentre(46, 22));
    }

protected:
    void paintContent(juce::Graphics& g) override
    {
        auto c = getLocalBounds().reduced(kPad);
        g.setColour(palette_.textPrimary);
        g.setFont(fonts::display(18.0f));
        g.drawText("AUDIO ENGINE", c.removeFromTop(28), juce::Justification::centredLeft, false);
        g.setColour(palette_.textTertiary);
        g.setFont(fonts::body(11.0f));
        g.drawText("Core DSP and hardware interfacing.", c.removeFromTop(18),
                   juce::Justification::topLeft, false);

        auto rowLabel = [&](juce::Rectangle<int> r, const char* t)
        {
            g.setColour(palette_.textSecondary);
            g.setFont(fonts::body(13.0f));
            g.drawText(t, r, juce::Justification::centredLeft, false);
        };
        rowLabel(c.removeFromTop(28), "Hardware Acceleration");
        c.removeFromTop(10);
        rowLabel(c.removeFromTop(28), "Low Latency Mode");
        c.removeFromTop(18);
        rowLabel(c.removeFromTop(18), "Sample Rate");
        c.removeFromTop(32);
        c.removeFromTop(18);
        auto bl = c.removeFromTop(18);
        g.setColour(palette_.textSecondary);
        g.setFont(fonts::body(13.0f));
        g.drawText("Buffer Size", bl, juce::Justification::centredLeft, false);
        g.setColour(palette_.textTertiary);
        g.setFont(fonts::mono(11.0f));
        g.drawText(juce::String(cfg_.bufferSize) + " smp", bl, juce::Justification::centredRight, false);

        c.removeFromTop(24); // buffer slider
        c.removeFromTop(16);
        auto rr = c.removeFromTop(28);
        g.setColour(palette_.textSecondary);
        g.setFont(fonts::body(13.0f));
        g.drawText("Reference Mode", rr.removeFromLeft(rr.getWidth() - 56),
                   juce::Justification::centredLeft, false);
        g.setColour(palette_.textTertiary);
        g.setFont(fonts::body(11.0f));
        g.drawText("Bypass all coloration — flat A/B", c.removeFromTop(16),
                   juce::Justification::topLeft, false);
    }

private:
    void emit() { if (onChanged) onChanged(cfg_); }
    static constexpr int kPad = 24;
    static constexpr int kRates[4] = {44100, 48000, 96000, 192000};

    veyra::AudioEngineConfig cfg_;
    ToggleSwitch     hwAccel_, lowLatency_, reference_;
    SegmentedControl rate_;
    juce::Slider     buffer_;
};

// ===========================================================================
// Updates
// ===========================================================================
class SettingsScreen::UpdatesCard : public GlassPanel {
public:
    UpdatesCard()
    {
        releases_.setButtonText("View Releases on GitHub");
        releases_.onClick = []
        { juce::URL("https://github.com/NextGenDev-KSK/veyra/releases").launchInDefaultBrowser(); };
        addAndMakeVisible(releases_);
    }

    void resized() override
    {
        auto c = getLocalBounds().reduced(kPad);
        c.removeFromTop(28 + 18 + 26 + 22 + 18); // title + 3 painted lines
        releases_.setBounds(c.removeFromTop(34).removeFromLeft(240));
    }

protected:
    void paintContent(juce::Graphics& g) override
    {
        auto c = getLocalBounds().reduced(kPad);
        g.setColour(palette_.textPrimary);
        g.setFont(fonts::display(18.0f));
        g.drawText("UPDATES", c.removeFromTop(28), juce::Justification::centredLeft, false);

        auto row = [&](const char* label, const juce::String& value)
        {
            auto r = c.removeFromTop(26);
            g.setColour(palette_.textSecondary);
            g.setFont(fonts::body(13.0f));
            g.drawText(label, r.removeFromLeft(160), juce::Justification::centredLeft, false);
            g.setColour(palette_.textPrimary);
            g.setFont(fonts::mono(12.0f));
            g.drawText(value, r, juce::Justification::centredLeft, false);
        };
        c.removeFromTop(18);
        row("Current version", "v" + juce::String(veyra::kVersionString));
        row("Channel", "Stable");
        c.removeFromTop(18);
        g.setColour(palette_.textTertiary);
        g.setFont(fonts::body(11.0f));
        g.drawText("Veyra checks for updates automatically in the background.",
                   c.removeFromTop(16), juce::Justification::topLeft, false);
    }

private:
    static constexpr int kPad = 24;
    juce::TextButton releases_;
};

// ===========================================================================
// Sound Quality: advanced enhancement controls beyond the six Home knobs.
// ===========================================================================
class SettingsScreen::SoundQualityCard : public GlassPanel {
public:
    std::function<void(float)> onExciterChanged;

    SoundQualityCard()
    {
        exciter_.setSliderStyle(juce::Slider::LinearHorizontal);
        exciter_.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
        exciter_.setRange(0.0, 1.0, 0.01);
        exciter_.onValueChange = [this]
        { if (onExciterChanged) onExciterChanged((float) exciter_.getValue()); repaint(); };
        addAndMakeVisible(exciter_);
    }

    void setExciter(float a)
    {
        exciter_.setValue(a, juce::dontSendNotification);
        repaint();
    }

    void resized() override
    {
        auto c = getLocalBounds().reduced(kPad);
        c.removeFromTop(28 + 18); // title + subtitle
        c.removeFromTop(18);      // exciter label row (painted)
        exciter_.setBounds(c.removeFromTop(24));
    }

protected:
    void paintContent(juce::Graphics& g) override
    {
        auto c = getLocalBounds().reduced(kPad);
        g.setColour(palette_.textPrimary);
        g.setFont(fonts::display(18.0f));
        g.drawText("SOUND QUALITY", c.removeFromTop(28), juce::Justification::centredLeft, false);
        g.setColour(palette_.textTertiary);
        g.setFont(fonts::body(11.0f));
        g.drawText("Subtle harmonic enhancement.", c.removeFromTop(18),
                   juce::Justification::topLeft, false);

        auto lr = c.removeFromTop(18);
        g.setColour(palette_.textSecondary);
        g.setFont(fonts::body(13.0f));
        g.drawText("Harmonic Exciter", lr.removeFromLeft(lr.getWidth() - 56),
                   juce::Justification::centredLeft, false);
        g.setColour(palette_.textTertiary);
        g.setFont(fonts::mono(11.0f));
        g.drawText(juce::String(juce::roundToInt(exciter_.getValue() * 100.0)) + "%", lr,
                   juce::Justification::centredRight, false);
    }

private:
    static constexpr int kPad = 24;
    juce::Slider exciter_;
};

SettingsScreen::SettingsScreen()
{
    appearance_ = std::make_unique<AppearanceCard>();
    appearance_->onThemeSelected  = [this](const juce::String& id) { if (onThemeSelected) onThemeSelected(id); };
    appearance_->onOpacity        = [this](double v) { if (onOpacity) onOpacity(v); };
    appearance_->onBackgroundMode = [this](int i) { if (onBackgroundMode) onBackgroundMode(i); };
    appearance_->onReduceMotion   = [this](bool b) { if (onReduceMotion) onReduceMotion(b); };
    addAndMakeVisible(*appearance_);

    audioEngine_ = std::make_unique<AudioEngineCard>();
    audioEngine_->onChanged = [this](const veyra::AudioEngineConfig& e) { if (onAudioEngineChanged) onAudioEngineChanged(e); };
    audioEngine_->onReferenceModeChanged = [this](bool on) { if (onReferenceModeChanged) onReferenceModeChanged(on); };
    addAndMakeVisible(*audioEngine_);

    microphone_ = std::make_unique<MicrophoneCard>();
    microphone_->onMicChanged = [this](const veyra::VoiceConfig& v) { if (onMicChanged) onMicChanged(v); };
    addAndMakeVisible(*microphone_);

    spatial_ = std::make_unique<SpatialCard>();
    spatial_->onSpatialChanged = [this](const veyra::SpatialConfig& s) { if (onSpatialChanged) onSpatialChanged(s); };
    addAndMakeVisible(*spatial_);

    loudness_ = std::make_unique<LoudnessCard>();
    loudness_->onLoudnessChanged = [this](const veyra::LoudnessConfig& l) { if (onLoudnessChanged) onLoudnessChanged(l); };
    addAndMakeVisible(*loudness_);

    soundQuality_ = std::make_unique<SoundQualityCard>();
    soundQuality_->onExciterChanged = [this](float a) { if (onExciterChanged) onExciterChanged(a); };
    addAndMakeVisible(*soundQuality_);

    updates_ = std::make_unique<UpdatesCard>();
    addAndMakeVisible(*updates_);

    about_ = std::make_unique<AboutCard>();
    about_->onResetSettings = [this] { if (onResetSettings) onResetSettings(); };
    addAndMakeVisible(*about_);

    setSection(0);
}

juce::Component* SettingsScreen::cardForSection(int i) const
{
    switch (i)
    {
    case 0: return appearance_.get();
    case 1: return audioEngine_.get();
    case 2: return microphone_.get();
    case 3: return spatial_.get();
    case 4: return loudness_.get();
    case 5: return soundQuality_.get();
    case 6: return updates_.get();
    default: return about_.get();
    }
}

void SettingsScreen::setSection(int i)
{
    section_ = juce::jlimit(0, kSections - 1, i);
    for (int s = 0; s < kSections; ++s)
        if (auto* c = cardForSection(s))
            c->setVisible(s == section_);
    resized();
    repaint();
}

juce::Rectangle<int> SettingsScreen::navItemBounds(int i) const
{
    auto content = getLocalBounds().reduced(24);
    content.removeFromTop(56);
    auto nav = content.removeFromLeft(180);
    return { nav.getX(), nav.getY() + i * 44, nav.getWidth() - 12, 38 };
}

void SettingsScreen::mouseDown(const juce::MouseEvent& e)
{
    for (int i = 0; i < kSections; ++i)
        if (navItemBounds(i).contains(e.getPosition()))
        {
            setSection(i);
            return;
        }
}

void SettingsScreen::paint(juce::Graphics& g)
{
    auto content = getLocalBounds().reduced(24);
    auto header = content.removeFromTop(56);
    g.setColour(palette_.textPrimary);
    g.setFont(fonts::display(28.0f));
    g.drawText("Settings", header.removeFromTop(34), juce::Justification::topLeft, false);
    g.setColour(palette_.textSecondary);
    g.setFont(fonts::body(13.0f));
    g.drawText("Configure application preferences and the audio engine.", header,
               juce::Justification::topLeft, false);

    static const char* kNames[kSections] =
        {"Appearance", "Audio Engine", "Microphone", "Spatial", "Loudness", "Sound Quality", "Updates", "About"};
    for (int i = 0; i < kSections; ++i)
    {
        if (i == section_)
        {
            auto rr = navItemBounds(i).toFloat();
            g.setColour(palette_.bgGlassActive);
            g.fillRoundedRectangle(rr, 8.0f);
            g.setColour(palette_.accentPrimary);
            g.fillRoundedRectangle(rr.removeFromLeft(3.0f), 1.5f); // accent rail
            g.setColour(palette_.textPrimary);
        }
        else
        {
            g.setColour(palette_.textSecondary);
        }
        g.setFont(fonts::body(14.0f, i == section_));
        g.drawText(kNames[i], navItemBounds(i).withTrimmedLeft(14),
                   juce::Justification::centredLeft, false);
    }
}

SettingsScreen::~SettingsScreen() = default;

void SettingsScreen::setPalette(const Palette& p)
{
    palette_ = p;
    appearance_->setPalette(p);
    audioEngine_->setPalette(p);
    microphone_->setPalette(p);
    spatial_->setPalette(p);
    loudness_->setPalette(p);
    soundQuality_->setPalette(p);
    updates_->setPalette(p);
    about_->setPalette(p);
    repaint();
}

void SettingsScreen::attachBackdrop(GlassBackground* b)
{
    appearance_->setBackdrop(b);
    audioEngine_->setBackdrop(b);
    microphone_->setBackdrop(b);
    spatial_->setBackdrop(b);
    loudness_->setBackdrop(b);
    soundQuality_->setBackdrop(b);
    updates_->setBackdrop(b);
    about_->setBackdrop(b);
}

void SettingsScreen::setCurrentTheme(const juce::String& id) { appearance_->setCurrentTheme(id); }
void SettingsScreen::setAppearance(double opacity, int backgroundMode, bool reduceMotion)
{
    appearance_->setAppearance(opacity, backgroundMode, reduceMotion);
}
void SettingsScreen::setMicConfig(const veyra::VoiceConfig& v) { microphone_->setMicConfig(v); }
void SettingsScreen::setSpatialConfig(const veyra::SpatialConfig& s) { spatial_->setSpatialConfig(s); }
void SettingsScreen::setLoudnessConfig(const veyra::LoudnessConfig& l) { loudness_->setLoudnessConfig(l); }
void SettingsScreen::setAudioEngineConfig(const veyra::AudioEngineConfig& e) { audioEngine_->setConfig(e); }
void SettingsScreen::setReferenceMode(bool on) { audioEngine_->setReferenceMode(on); }
void SettingsScreen::setExciter(float a) { soundQuality_->setExciter(a); }
void SettingsScreen::setServiceStatus(bool connected, juce::String version)
{
    about_->setServiceStatus(connected, std::move(version));
}

void SettingsScreen::resized()
{
    // Title + left sub-nav (painted); the selected card fills the right area.
    auto content = getLocalBounds().reduced(24);
    content.removeFromTop(56);   // header (painted)
    content.removeFromLeft(180); // sub-nav (painted)
    content.removeFromLeft(24);  // gutter
    const auto cardArea = content;

    for (int s = 0; s < kSections; ++s)
        if (auto* c = cardForSection(s))
            c->setBounds(cardArea);
}

} // namespace veyra::ui
