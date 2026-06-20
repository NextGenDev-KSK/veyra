#include "Screens/DevicesScreen.h"

#include "AudioDevices.h"
#include "Components/GlassPanel.h"
#include "Components/ToggleSwitch.h"
#include "Graphics/GlassBackground.h"
#include "Theme/Fonts.h"

#include <vector>

namespace veyra::ui {

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

DevicesScreen::DevicesScreen()
{
    card_ = std::make_unique<BridgeCard>();
    card_->onBridgeChanged = [this](const veyra::BridgeConfig& b) { if (onBridgeChanged) onBridgeChanged(b); };
    addAndMakeVisible(*card_);
}

DevicesScreen::~DevicesScreen() = default;

void DevicesScreen::setPalette(const Palette& p)       { card_->setPalette(p); }
void DevicesScreen::attachBackdrop(GlassBackground* b) { card_->setBackdrop(b); }
void DevicesScreen::refreshDevices()                   { card_->refreshDevices(); }
void DevicesScreen::setBridge(const veyra::BridgeConfig& b) { card_->setBridge(b); }

void DevicesScreen::resized()
{
    card_->setBounds(getLocalBounds().reduced(24));
}

} // namespace veyra::ui
