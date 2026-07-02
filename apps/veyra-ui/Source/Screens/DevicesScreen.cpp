#include "Screens/DevicesScreen.h"

#include "AudioDevices.h"
#include "Components/GlassPanel.h"
#include "Components/ToggleSwitch.h"
#include "Graphics/GlassBackground.h"
#include "Graphics/Icons.h"
#include "Theme/Fonts.h"

#include <vector>

namespace veyra::ui {

namespace {
constexpr int kCardW = 244, kCardH = 132, kGapX = 16, kGapY = 16;
constexpr int kLabelH = 22, kSectionGap = 10, kBridgeH = 150;
} // namespace

// ---------------------------------------------------------------------------
// Read-only device card (overview): icon + name + type badge + status + a detail
// row (PRESET / PROFILE) and an optional level/volume bar.
// ---------------------------------------------------------------------------
class DevicesScreen::DeviceCard : public GlassPanel
{
public:
    void setData(juce::String name, juce::String badge, juce::String status, juce::Colour statusCol,
                 juce::String detailLabel, juce::String detailValue, float bar)
    {
        name_ = std::move(name); badge_ = std::move(badge); status_ = std::move(status);
        statusCol_ = statusCol; detailLabel_ = std::move(detailLabel);
        detailValue_ = std::move(detailValue); bar_ = bar;
        repaint();
    }

protected:
    void paintContent(juce::Graphics& g) override
    {
        auto c = getLocalBounds().reduced(16);
        auto top = c.removeFromTop(38);

        // Icon chip + device glyph (accented on the active device).
        auto chip = top.removeFromLeft(38).toFloat();
        g.setColour(palette_.bgInput);
        g.fillRoundedRectangle(chip, 9.0f);
        icons::devices(g, chip.reduced(9.0f),
                       bar_ >= 0.0f || detailLabel_.isNotEmpty() ? palette_.accentPrimary
                                                                 : palette_.textSecondary);

        // Type badge (top-right pill).
        if (badge_.isNotEmpty())
        {
            g.setColour(palette_.bgGlassHover);
            auto pill = juce::Rectangle<int>(top.getRight() - 64, top.getY() + 4, 64, 16);
            g.fillRoundedRectangle(pill.toFloat(), 8.0f);
            g.setColour(palette_.textSecondary);
            g.setFont(fonts::mono(8.5f, true));
            g.drawText(badge_.toUpperCase(), pill, juce::Justification::centred, false);
        }

        c.removeFromTop(8);
        g.setColour(palette_.textPrimary);
        g.setFont(fonts::display(15.0f));
        g.drawText(name_, c.removeFromTop(22), juce::Justification::topLeft, true);

        auto st = c.removeFromTop(16);
        g.setColour(statusCol_);
        g.fillEllipse((float) st.getX(), st.getCentreY() - 3.0f, 6.0f, 6.0f);
        g.setColour(palette_.textSecondary);
        g.setFont(fonts::body(11.0f));
        g.drawText(status_, st.withTrimmedLeft(12), juce::Justification::centredLeft, false);

        if (detailLabel_.isNotEmpty())
        {
            c.removeFromTop(8);
            auto row = c.removeFromTop(16);
            g.setColour(palette_.textTertiary);
            g.setFont(fonts::mono(9.0f, true));
            g.drawText(detailLabel_, row.removeFromLeft(60), juce::Justification::centredLeft, false);
            g.setColour(palette_.accentPrimary);
            g.setFont(fonts::body(12.0f, true));
            g.drawText(detailValue_, row, juce::Justification::centredRight, true);
        }

        if (bar_ >= 0.0f)
        {
            c.removeFromTop(8);
            auto m = c.removeFromTop(8);
            g.setColour(palette_.bgInput);
            g.fillRoundedRectangle(m.toFloat(), 3.0f);
            g.setColour(palette_.accentSecondary);
            g.fillRoundedRectangle(m.toFloat().withWidth(m.getWidth() * juce::jlimit(0.0f, 1.0f, bar_)), 3.0f);
        }
    }

private:
    juce::String name_, badge_, status_, detailLabel_, detailValue_;
    juce::Colour statusCol_ = juce::Colours::grey;
    float        bar_ = -1.0f;
};

// ---------------------------------------------------------------------------
// Scrollable content: OUTPUT + INPUT device-card grids (the Viewport scrolls
// this when there are more endpoints than fit). Computes its own height so the
// rows never collide with each other or the Bridge below.
// ---------------------------------------------------------------------------
class DevicesScreen::CardGrid : public juce::Component
{
public:
    void setPalette(const Palette& p)
    {
        palette_ = p;
        for (auto& c : outCards_) c->setPalette(p);
        for (auto& c : inCards_)  c->setPalette(p);
        repaint();
    }
    void setBackdrop(GlassBackground* b)
    {
        backdrop_ = b;
        for (auto& c : outCards_) c->setBackdrop(b);
        for (auto& c : inCards_)  c->setBackdrop(b);
    }
    void setDevices(std::vector<OutputDevice> out, std::vector<OutputDevice> in)
    {
        outDevs_ = std::move(out); inDevs_ = std::move(in); rebuild();
    }
    void setActivePreset(juce::String n) { activePreset_ = n.isNotEmpty() ? n : "Custom"; rebuild(); }
    void setMasterVolume(double g)       { masterVol_ = g; rebuild(); }
    void setMicProfile(juce::String p)   { micProfile_ = p.isNotEmpty() ? p : "gaming"; rebuild(); }

    int preferredHeight(int width) const
    {
        return kLabelH + rowsFor((int) outDevs_.size(), width) * (kCardH + kGapY)
             + kSectionGap + kLabelH + rowsFor((int) inDevs_.size(), width) * (kCardH + kGapY)
             + 4;
    }

    void resized() override { layout(); }

    void paint(juce::Graphics& g) override
    {
        const int rowsO = rowsFor((int) outDevs_.size(), getWidth());
        auto label = [&](int y, const char* t)
        {
            g.setColour(palette_.accentSecondary);
            g.setFont(fonts::mono(11.0f, true));
            g.drawText(t, juce::Rectangle<int>(0, y, getWidth(), kLabelH),
                       juce::Justification::centredLeft, false);
        };
        label(0, "OUTPUT DEVICES");
        label(kLabelH + rowsO * (kCardH + kGapY) + kSectionGap, "INPUT DEVICES");
    }

private:
    int rowsFor(int count, int width) const
    {
        if (count <= 0) return 0;
        const int cols = juce::jmax(1, width / (kCardW + kGapX));
        return (count + cols - 1) / cols;
    }

    void place(std::vector<std::unique_ptr<DeviceCard>>& cards, int top, int cols)
    {
        for (int i = 0; i < (int) cards.size(); ++i)
            cards[(size_t) i]->setBounds((i % cols) * (kCardW + kGapX),
                                         top + (i / cols) * (kCardH + kGapY), kCardW, kCardH);
    }

    void layout()
    {
        const int w = getWidth();
        if (w <= 0) return;
        const int cols = juce::jmax(1, w / (kCardW + kGapX));
        place(outCards_, kLabelH, cols);
        const int afterOut = kLabelH + rowsFor((int) outDevs_.size(), w) * (kCardH + kGapY);
        place(inCards_, afterOut + kSectionGap + kLabelH, cols);
    }

    void rebuild()
    {
        outCards_.clear();
        inCards_.clear();
        auto add = [this](std::vector<std::unique_ptr<DeviceCard>>& into, const OutputDevice& d, bool input)
        {
            auto card = std::make_unique<DeviceCard>();
            card->setPalette(palette_);
            card->setBackdrop(backdrop_);
            const juce::String badge = d.type.isNotEmpty() ? d.type : (input ? "Input" : "Output");
            if (d.isDefault)
                card->setData(d.name, badge, input ? "Active Input" : "Active Output", palette_.success,
                              input ? "PROFILE" : "PRESET",
                              input ? micProfile_ : activePreset_,
                              input ? 0.35f : (float) juce::jlimit(0.0, 1.0, masterVol_ / 1.5));
            else
                card->setData(d.name, badge, input ? "Standby" : "Available", palette_.textTertiary,
                              {}, {}, -1.0f);
            addAndMakeVisible(*card);
            into.push_back(std::move(card));
        };
        for (const auto& d : outDevs_) add(outCards_, d, false);
        for (const auto& d : inDevs_)  add(inCards_, d, true);

        // Update our own height for the new card count, then lay the cards out
        // (setSize is a no-op when the count is unchanged, so position regardless).
        setSize(getWidth(), preferredHeight(getWidth()));
        layout();
    }

    std::vector<std::unique_ptr<DeviceCard>> outCards_, inCards_;
    std::vector<OutputDevice> outDevs_, inDevs_;
    Palette          palette_  = paletteForTheme("midnight");
    GlassBackground* backdrop_ = nullptr;
    juce::String     activePreset_{"Custom"};
    double           masterVol_ = 1.0;
    juce::String     micProfile_{"gaming"};
};

// ---------------------------------------------------------------------------
// Output device (APO / Method 1): the Preferred Output selector. The APO
// processes the Windows default output system-wide, so there is no source/capture
// and no Audio Bridge — picking a preferred device just keeps it as the default.
// ---------------------------------------------------------------------------
class DevicesScreen::BridgeCard : public GlassPanel {
public:
    BridgeCard()
    {
        preferred_.setTextWhenNothingSelected("Windows Default");
        addAndMakeVisible(preferred_);
        preferred_.onChange = [this] { bridge_.preferredOutputId = idForPreferred(); emit(); };

        refresh_.setButtonText("Refresh");
        refresh_.onClick = [this] { refreshDevices(); };
        addAndMakeVisible(refresh_);

        refreshDevices();
    }

    std::function<void(const veyra::BridgeConfig&)> onBridgeChanged;

    void refreshDevices()
    {
        devices_ = listRenderEndpoints();
        // Preferred Output: item 1 = "Windows Default (no preference)", devices 2..n+1.
        preferred_.clear(juce::dontSendNotification);
        preferred_.addItem("Windows Default (no preference)", 1);
        for (int i = 0; i < (int) devices_.size(); ++i)
            preferred_.addItem(devices_[(size_t) i].name + (devices_[(size_t) i].isDefault ? "  (current default)" : ""),
                               i + 2);
        selectIds();
        repaint();
    }

    void setBridge(const veyra::BridgeConfig& b)
    {
        bridge_ = b;
        selectIds();
        repaint();
    }

    void resized() override
    {
        auto inner = getLocalBounds().reduced(kPad);
        const int comboW = juce::jmin(360, getWidth() - kPad * 2 - 84 - 110);
        preferred_.setBounds(rowCtl(inner.getX(), 1, comboW));
        // Refresh sits to the right of the picker (never over the label/row).
        refresh_.setBounds(inner.getX() + 84 + comboW + 16, rowCtl(inner.getX(), 1, comboW).getY(), 90, 28);
    }

protected:
    void paintContent(juce::Graphics& g) override
    {
        auto c = getLocalBounds().reduced(kPad);
        g.setColour(palette_.textPrimary);
        g.setFont(fonts::display(20.0f));
        g.drawText("OUTPUT DEVICE", c.removeFromTop(28), juce::Justification::centredLeft, false);

        g.setColour(palette_.textTertiary);
        g.setFont(fonts::body(11.0f));
        g.drawText("Pick a Preferred Output; Veyra keeps it as the Windows default automatically.",
                   c.removeFromTop(16), juce::Justification::topLeft, false);
        g.drawText("APO builds process it system-wide; unsigned builds use the Audio Bridge (see the guide).",
                   c.removeFromTop(16), juce::Justification::topLeft, false);

        const int y = getLocalBounds().reduced(kPad).getY() + 28 + 40;
        g.setColour(palette_.textSecondary);
        g.setFont(fonts::body(13.0f));
        g.drawText("Preferred", juce::Rectangle<int>(getLocalBounds().reduced(kPad).getX(), y, 84, 28),
                   juce::Justification::centredLeft, false);
    }

private:
    juce::Rectangle<int> rowCtl(int x, int row, int w) const
    {
        const int y = getLocalBounds().reduced(kPad).getY() + 28 + 40 + (row - 1) * 40;
        return {x + 84, y, juce::jmax(120, w), 28};
    }

    // Preferred uses item 1 = no preference (empty), devices at id 2..n+1.
    std::string idForPreferred() const
    {
        const int idx = preferred_.getSelectedId() - 2;
        return (idx >= 0 && idx < (int) devices_.size()) ? devices_[(size_t) idx].id : std::string();
    }

    void selectIds()
    {
        // Preferred: id 1 when no preference, else device index + 2.
        if (bridge_.preferredOutputId.empty())
            preferred_.setSelectedId(1, juce::dontSendNotification);
        else
        {
            int sel = 1;
            for (int i = 0; i < (int) devices_.size(); ++i)
                if (devices_[(size_t) i].id == bridge_.preferredOutputId) { sel = i + 2; break; }
            preferred_.setSelectedId(sel, juce::dontSendNotification);
        }
    }

    void emit()
    {
        if (onBridgeChanged)
            onBridgeChanged(bridge_);
        repaint();
    }

    static constexpr int kPad = 24;

    std::vector<OutputDevice> devices_;
    veyra::BridgeConfig       bridge_;
    juce::ComboBox            preferred_;
    juce::TextButton          refresh_;
};

// ---------------------------------------------------------------------------

DevicesScreen::DevicesScreen()
{
    grid_ = std::make_unique<CardGrid>();
    viewport_.setViewedComponent(grid_.get(), false);
    viewport_.setScrollBarsShown(true, false); // vertical only
    addAndMakeVisible(viewport_);

    card_ = std::make_unique<BridgeCard>();
    card_->onBridgeChanged = [this](const veyra::BridgeConfig& b) { if (onBridgeChanged) onBridgeChanged(b); };
    addAndMakeVisible(*card_);

    refreshDevices();
}

DevicesScreen::~DevicesScreen() = default;

void DevicesScreen::setPalette(const Palette& p)
{
    palette_ = p;
    grid_->setPalette(p);
    card_->setPalette(p);
    repaint();
}

void DevicesScreen::attachBackdrop(GlassBackground* b)
{
    grid_->setBackdrop(b);
    card_->setBackdrop(b);
}

void DevicesScreen::refreshDevices()
{
    grid_->setDevices(listRenderEndpoints(), listCaptureEndpoints());
    card_->refreshDevices();
    resized();
}
void DevicesScreen::setBridge(const veyra::BridgeConfig& b) { card_->setBridge(b); }
void DevicesScreen::setActivePreset(juce::String n) { grid_->setActivePreset(n); }
void DevicesScreen::setMasterVolume(double g)       { grid_->setMasterVolume(g); }
void DevicesScreen::setMicProfile(juce::String p)   { grid_->setMicProfile(p); }

void DevicesScreen::paint(juce::Graphics& g)
{
    auto c = getLocalBounds().reduced(24);
    g.setColour(palette_.textPrimary);
    g.setFont(fonts::display(24.0f));
    g.drawText("DEVICE ROUTING", c.removeFromTop(34), juce::Justification::centredLeft, false);
}

void DevicesScreen::resized()
{
    auto b = getLocalBounds().reduced(24);
    b.removeFromTop(34 + 10); // title + gap

    card_->setBounds(b.removeFromBottom(kBridgeH));
    b.removeFromBottom(12);   // gap above the bridge
    viewport_.setBounds(b);

    // Size the scrollable content: reserve scrollbar width only when it overflows.
    int w = viewport_.getWidth();
    if (w <= 0) return;
    int h = grid_->preferredHeight(w);
    if (h > viewport_.getHeight())
        w -= viewport_.getScrollBarThickness();
    grid_->setSize(w, grid_->preferredHeight(w));
}

} // namespace veyra::ui
