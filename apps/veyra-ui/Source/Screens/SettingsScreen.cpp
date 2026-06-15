#include "Screens/SettingsScreen.h"

#include "Components/GlassPanel.h"
#include "Components/SegmentedControl.h"
#include "Components/ToggleSwitch.h"
#include "Graphics/GlassBackground.h"
#include "Theme/Fonts.h"

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
// SettingsScreen: thin host that forwards to the card.
// ---------------------------------------------------------------------------
SettingsScreen::SettingsScreen()
{
    appearance_ = std::make_unique<AppearanceCard>();
    appearance_->onThemeSelected  = [this](const juce::String& id) { if (onThemeSelected) onThemeSelected(id); };
    appearance_->onOpacity        = [this](double v) { if (onOpacity) onOpacity(v); };
    appearance_->onBackgroundMode = [this](int i) { if (onBackgroundMode) onBackgroundMode(i); };
    appearance_->onReduceMotion   = [this](bool b) { if (onReduceMotion) onReduceMotion(b); };
    addAndMakeVisible(*appearance_);
}

SettingsScreen::~SettingsScreen() = default;

void SettingsScreen::setPalette(const Palette& p)       { appearance_->setPalette(p); }
void SettingsScreen::attachBackdrop(GlassBackground* b) { appearance_->setBackdrop(b); }
void SettingsScreen::setCurrentTheme(const juce::String& id) { appearance_->setCurrentTheme(id); }

void SettingsScreen::resized()
{
    appearance_->setBounds(getLocalBounds());
}

} // namespace veyra::ui
