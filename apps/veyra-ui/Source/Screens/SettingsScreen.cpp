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
    }

    std::function<void(const veyra::LoudnessConfig&)> onLoudnessChanged;

    void setPalette(const Palette& p) override
    {
        GlassPanel::setPalette(p);
        sleepEnable_.setPalette(p);
    }

    void setLoudnessConfig(const veyra::LoudnessConfig& l)
    {
        loud_ = l;
        night_.setValue(l.nightModeAmount, juce::dontSendNotification);
        sleepEnable_.setToggleState(l.sleepTimerEnabled, juce::dontSendNotification);
        minutes_.setValue(l.sleepTimerMinutes, juce::dontSendNotification);
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
SettingsScreen::SettingsScreen()
{
    appearance_ = std::make_unique<AppearanceCard>();
    appearance_->onThemeSelected  = [this](const juce::String& id) { if (onThemeSelected) onThemeSelected(id); };
    appearance_->onOpacity        = [this](double v) { if (onOpacity) onOpacity(v); };
    appearance_->onBackgroundMode = [this](int i) { if (onBackgroundMode) onBackgroundMode(i); };
    appearance_->onReduceMotion   = [this](bool b) { if (onReduceMotion) onReduceMotion(b); };
    addAndMakeVisible(*appearance_);

    microphone_ = std::make_unique<MicrophoneCard>();
    microphone_->onMicChanged = [this](const veyra::VoiceConfig& v) { if (onMicChanged) onMicChanged(v); };
    addAndMakeVisible(*microphone_);

    spatial_ = std::make_unique<SpatialCard>();
    spatial_->onSpatialChanged = [this](const veyra::SpatialConfig& s) { if (onSpatialChanged) onSpatialChanged(s); };
    addAndMakeVisible(*spatial_);

    loudness_ = std::make_unique<LoudnessCard>();
    loudness_->onLoudnessChanged = [this](const veyra::LoudnessConfig& l) { if (onLoudnessChanged) onLoudnessChanged(l); };
    addAndMakeVisible(*loudness_);

    about_ = std::make_unique<AboutCard>();
    about_->onResetSettings = [this] { if (onResetSettings) onResetSettings(); };
    addAndMakeVisible(*about_);
}

SettingsScreen::~SettingsScreen() = default;

void SettingsScreen::setPalette(const Palette& p)
{
    appearance_->setPalette(p);
    microphone_->setPalette(p);
    spatial_->setPalette(p);
    loudness_->setPalette(p);
    about_->setPalette(p);
}

void SettingsScreen::attachBackdrop(GlassBackground* b)
{
    appearance_->setBackdrop(b);
    microphone_->setBackdrop(b);
    spatial_->setBackdrop(b);
    loudness_->setBackdrop(b);
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
void SettingsScreen::setServiceStatus(bool connected, juce::String version)
{
    about_->setServiceStatus(connected, std::move(version));
}

void SettingsScreen::resized()
{
    auto b = getLocalBounds().reduced(24);

    // Bottom strip: Loudness (left) + About (right).
    auto bottom = b.removeFromBottom(176);
    b.removeFromBottom(20);
    auto loudnessArea = bottom.removeFromLeft(juce::jlimit(300, 460, bottom.getWidth() * 2 / 5));
    bottom.removeFromLeft(20);
    loudness_->setBounds(loudnessArea);
    about_->setBounds(bottom);

    // Above it: Appearance (wide, theme grid) left; Microphone over Spatial right.
    const int rightW = juce::jlimit(300, 420, b.getWidth() / 3);
    auto rightCol = b.removeFromRight(rightW);
    b.removeFromRight(20); // gutter
    appearance_->setBounds(b);

    auto spatialArea = rightCol.removeFromBottom(juce::jlimit(160, 220, rightCol.getHeight() / 3));
    rightCol.removeFromBottom(20);
    microphone_->setBounds(rightCol);
    spatial_->setBounds(spatialArea);
}

} // namespace veyra::ui
