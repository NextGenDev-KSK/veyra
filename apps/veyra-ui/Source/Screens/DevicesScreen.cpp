#include "Screens/DevicesScreen.h"

#include "AudioDevices.h"
#include "Components/GlassPanel.h"
#include "Components/ToggleSwitch.h"
#include "Graphics/GlassBackground.h"
#include "Theme/Fonts.h"

#include <vector>

namespace veyra::ui {

namespace {
constexpr int kCardW = 244, kCardH = 132, kGapX = 16, kGapY = 16;
} // namespace

class DevicesScreen::BridgeCard : public GlassPanel {
public:
    BridgeCard()
    {
        enable_.onClick = [this] { bridge_.enabled = enable_.getToggleState(); emit(); };
        addAndMakeVisible(enable_);

        for (auto* c : {&source_, &target_})
        {
            c->setTextWhenNothingSelected("(select device)");
            addAndMakeVisible(c);
        }
        source_.onChange = [this] { bridge_.sourceDeviceId = idFor(source_); emit(); };
        target_.onChange = [this] { bridge_.targetDeviceId = idFor(target_); emit(); };

        refresh_.setButtonText("Refresh");
        refresh_.onClick = [this] { refreshDevices(); };
        addAndMakeVisible(refresh_);

        refreshDevices();
    }

    std::function<void(const veyra::BridgeConfig&)> onBridgeChanged;

    void setPalette(const Palette& p) override
    {
        GlassPanel::setPalette(p);
        enable_.setPalette(p);
    }

    void refreshDevices()
    {
        devices_ = listRenderEndpoints();
        for (auto* c : {&source_, &target_})
        {
            c->clear(juce::dontSendNotification);
            for (int i = 0; i < (int) devices_.size(); ++i)
                c->addItem(devices_[(size_t) i].name + (devices_[(size_t) i].isDefault ? "  (default)" : ""),
                           i + 1);
        }
        selectIds();
        repaint();
    }

    void setBridge(const veyra::BridgeConfig& b)
    {
        bridge_ = b;
        enable_.setToggleState(b.enabled, juce::dontSendNotification);
        selectIds();
        repaint();
    }

    void resized() override
    {
        auto c = getLocalBounds().reduced(kPad);
        auto header = c.removeFromTop(28);
        enable_.setBounds(header.removeFromRight(40).withSizeKeepingCentre(40, 20));

        c.removeFromTop(40);                      // explainer (painted)
        source_.setBounds(rowCtl(c.getX(), 1));
        target_.setBounds(rowCtl(c.getX(), 2));
        refresh_.setBounds(getLocalBounds().reduced(kPad).removeFromBottom(30).removeFromLeft(90));
    }

protected:
    void paintContent(juce::Graphics& g) override
    {
        auto c = getLocalBounds().reduced(kPad);
        g.setColour(palette_.textPrimary);
        g.setFont(fonts::display(20.0f));
        g.drawText("AUDIO BRIDGE", c.removeFromTop(28), juce::Justification::centredLeft, false);

        g.setColour(palette_.textTertiary);
        g.setFont(fonts::body(11.0f));
        g.drawText("Process any app's audio to any output (e.g. Bluetooth).",
                   c.removeFromTop(16), juce::Justification::topLeft, false);
        g.drawText("Set Source as your Windows default; pick your headphones as Target.",
                   c.removeFromTop(16), juce::Justification::topLeft, false);

        auto labelAt = [&](int row, const char* txt)
        {
            const int y = getLocalBounds().reduced(kPad).getY() + 28 + 40 + (row - 1) * 40;
            g.setColour(palette_.textSecondary);
            g.setFont(fonts::body(13.0f));
            g.drawText(txt, juce::Rectangle<int>(getLocalBounds().reduced(kPad).getX(), y, 80, 28),
                       juce::Justification::centredLeft, false);
        };
        labelAt(1, "Source");
        labelAt(2, "Output");
    }

private:
    juce::Rectangle<int> rowCtl(int x, int row) const
    {
        const int y = getLocalBounds().reduced(kPad).getY() + 28 + 40 + (row - 1) * 40;
        return {x + 84, y, juce::jmin(360, getWidth() - kPad * 2 - 84), 28};
    }

    std::string idFor(const juce::ComboBox& box) const
    {
        const int idx = box.getSelectedId() - 1;
        return (idx >= 0 && idx < (int) devices_.size()) ? devices_[(size_t) idx].id : std::string();
    }

    void selectIds()
    {
        auto select = [this](juce::ComboBox& box, const std::string& id)
        {
            for (int i = 0; i < (int) devices_.size(); ++i)
                if (devices_[(size_t) i].id == id)
                { box.setSelectedId(i + 1, juce::dontSendNotification); return; }
            box.setSelectedId(0, juce::dontSendNotification);
        };
        select(source_, bridge_.sourceDeviceId);
        select(target_, bridge_.targetDeviceId);
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
    ToggleSwitch              enable_;
    juce::ComboBox            source_, target_;
    juce::TextButton          refresh_;
};

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

        // Icon chip.
        auto chip = top.removeFromLeft(38).toFloat();
        g.setColour(palette_.bgInput);
        g.fillRoundedRectangle(chip, 9.0f);

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

DevicesScreen::DevicesScreen()
{
    card_ = std::make_unique<BridgeCard>();
    card_->onBridgeChanged = [this](const veyra::BridgeConfig& b) { if (onBridgeChanged) onBridgeChanged(b); };
    addAndMakeVisible(*card_);
    refreshDevices();
}

DevicesScreen::~DevicesScreen() = default;

void DevicesScreen::rebuildCards()
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
        {
            card->setData(d.name, badge, input ? "Active Input" : "Active Output", palette_.success,
                          input ? "PROFILE" : "PRESET",
                          input ? micProfile_ : activePreset_,
                          input ? 0.35f : (float) juce::jlimit(0.0, 1.0, masterVol_ / 1.5));
        }
        else
        {
            card->setData(d.name, badge, input ? "Standby" : "Available", palette_.textTertiary,
                          {}, {}, -1.0f);
        }
        addAndMakeVisible(*card);
        into.push_back(std::move(card));
    };

    for (const auto& d : outDevs_) add(outCards_, d, false);
    for (const auto& d : inDevs_)  add(inCards_, d, true);
    resized();
}

void DevicesScreen::setPalette(const Palette& p)
{
    palette_ = p;
    card_->setPalette(p);
    for (auto& c : outCards_) c->setPalette(p);
    for (auto& c : inCards_) c->setPalette(p);
    repaint();
}

void DevicesScreen::attachBackdrop(GlassBackground* b)
{
    backdrop_ = b;
    card_->setBackdrop(b);
    for (auto& c : outCards_) c->setBackdrop(b);
    for (auto& c : inCards_) c->setBackdrop(b);
}

void DevicesScreen::refreshDevices()
{
    outDevs_ = listRenderEndpoints();
    inDevs_  = listCaptureEndpoints();
    card_->refreshDevices();
    rebuildCards();
}
void DevicesScreen::setBridge(const veyra::BridgeConfig& b) { card_->setBridge(b); }
void DevicesScreen::setActivePreset(juce::String n) { activePreset_ = n.isNotEmpty() ? n : "Custom"; rebuildCards(); }
void DevicesScreen::setMasterVolume(double g)       { masterVol_ = g; rebuildCards(); }
void DevicesScreen::setMicProfile(juce::String p)   { micProfile_ = p.isNotEmpty() ? p : "gaming"; rebuildCards(); }

void DevicesScreen::paint(juce::Graphics& g)
{
    auto c = getLocalBounds().reduced(24);
    g.setColour(palette_.textPrimary);
    g.setFont(fonts::display(24.0f));
    g.drawText("DEVICE ROUTING", c.removeFromTop(34), juce::Justification::centredLeft, false);

    auto sectionLabel = [&](juce::Rectangle<int> r, const char* t)
    {
        g.setColour(palette_.accentSecondary);
        g.setFont(fonts::mono(11.0f, true));
        g.drawText(t, r, juce::Justification::centredLeft, false);
    };
    c.removeFromTop(10);
    sectionLabel(c.removeFromTop(22), "OUTPUT DEVICES");
    c.removeFromTop(kCardH + kGapY);
    sectionLabel(c.removeFromTop(22), "INPUT DEVICES");
}

void DevicesScreen::resized()
{
    auto c = getLocalBounds().reduced(24);
    c.removeFromTop(34 + 10); // title + gap

    auto layoutGrid = [&](std::vector<std::unique_ptr<DeviceCard>>& cards)
    {
        c.removeFromTop(22); // section label (painted)
        const int cols = juce::jmax(1, c.getWidth() / (kCardW + kGapX));
        for (int i = 0; i < (int) cards.size(); ++i)
        {
            const int col = i % cols, row = i / cols;
            cards[(size_t) i]->setBounds(c.getX() + col * (kCardW + kGapX),
                                         c.getY() + row * (kCardH + kGapY), kCardW, kCardH);
        }
        c.removeFromTop(kCardH + kGapY); // reserve one row (matches paint)
    };
    layoutGrid(outCards_);
    layoutGrid(inCards_);

    c.removeFromTop(8);
    card_->setBounds(c); // Audio Bridge (functional routing) fills the rest
}

} // namespace veyra::ui
