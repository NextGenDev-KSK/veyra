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
constexpr int kLabelH = 22, kSectionGap = 10, kBridgeH = 364;
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
// Audio routing card. The Audio Bridge is the audio path on unsigned builds:
// apps play into a virtual sink (VB-CABLE or similar), the service loopback-
// captures it, runs the DSP chain, and renders to the real output device. The
// Mic Bridge section below it does the same in reverse for the microphone
// (mic -> voice chain -> a second cable that apps use as their mic). The
// Preferred Output picker belongs to the signed-APO path and is only active
// while the Bridge is off.
// ---------------------------------------------------------------------------
class DevicesScreen::BridgeCard : public GlassPanel {
public:
    BridgeCard()
    {
        enable_.setClickingTogglesState(true);
        enable_.onClick = [this]
        {
            bridge_.enabled = enable_.getToggleState();
            if (bridge_.enabled)
                autoPickDevices();
            selectIds();
            emit();
        };
        addAndMakeVisible(enable_);

        source_.setTextWhenNothingSelected("Windows default output");
        source_.onChange = [this] { bridge_.sourceDeviceId = idForCombo(source_, true); emit(); };
        addAndMakeVisible(source_);

        target_.setTextWhenNothingSelected("Choose your headphones or speakers");
        target_.onChange = [this] { bridge_.targetDeviceId = idForCombo(target_, false); emit(); };
        addAndMakeVisible(target_);

        preferred_.setTextWhenNothingSelected("Windows Default");
        preferred_.onChange = [this] { bridge_.preferredOutputId = idForCombo(preferred_, true); emit(); };
        addAndMakeVisible(preferred_);

        micEnable_.setClickingTogglesState(true);
        micEnable_.onClick = [this]
        {
            mic_.enabled = micEnable_.getToggleState();
            if (mic_.enabled)
                autoPickMicDevices();
            selectIds();
            emitMic();
        };
        addAndMakeVisible(micEnable_);

        micSource_.setTextWhenNothingSelected("Default microphone");
        micSource_.onChange = [this] { mic_.micDeviceId = idForMicCombo(); emitMic(); };
        addAndMakeVisible(micSource_);

        micTarget_.setTextWhenNothingSelected("Choose the mic's cable");
        micTarget_.onChange = [this] { mic_.targetDeviceId = idForCombo(micTarget_, false); emitMic(); };
        addAndMakeVisible(micTarget_);

        refresh_.setButtonText("Refresh");
        refresh_.onClick = [this] { refreshDevices(); };
        addAndMakeVisible(refresh_);

        // Shown only while a bridge is on and no virtual cable exists.
        cable_.setButtonText("Get VB-CABLE");
        cable_.onClick = [] { juce::URL("https://vb-audio.com/Cable/").launchInDefaultBrowser(); };
        addChildComponent(cable_);

        refreshDevices();
    }

    std::function<void(const veyra::BridgeConfig&)>    onBridgeChanged;
    std::function<void(const veyra::MicBridgeConfig&)> onMicBridgeChanged;

    void refreshDevices()
    {
        devices_        = listRenderEndpoints();
        captureDevices_ = listCaptureEndpoints();

        auto fill = [](juce::ComboBox& box, const std::vector<OutputDevice>& list,
                       const char* defaultLabel)
        {
            box.clear(juce::dontSendNotification);
            if (defaultLabel != nullptr)
                box.addItem(defaultLabel, 1);
            for (int i = 0; i < (int) list.size(); ++i)
                box.addItem(list[(size_t) i].name
                                + (list[(size_t) i].isDefault ? "  (current default)" : ""),
                            i + 2);
        };
        fill(source_, devices_, "Windows default output");
        fill(target_, devices_, nullptr);
        fill(preferred_, devices_, "Windows Default (no preference)");
        fill(micSource_, captureDevices_, "Default microphone");
        fill(micTarget_, devices_, nullptr);

        selectIds();
        repaint();
    }

    void setBridge(const veyra::BridgeConfig& b)
    {
        bridge_ = b;
        selectIds();
        repaint();
    }

    void setMicBridge(const veyra::MicBridgeConfig& m)
    {
        mic_ = m;
        selectIds();
        repaint();
    }

    void resized() override
    {
        auto inner = getLocalBounds().reduced(kPad);
        // Toggles sit after their heading text; Refresh + the VB-CABLE link are
        // right-aligned in the title row.
        enable_.setBounds(inner.getX() + 156, inner.getY() + 4, 36, 20);
        refresh_.setBounds(inner.getRight() - 90, inner.getY(), 90, 28);
        cable_.setBounds(inner.getRight() - 90 - 10 - 110, inner.getY(), 110, 28);
        micEnable_.setBounds(inner.getX() + 156, micHeaderY() + 2, 36, 20);

        const int comboW = juce::jmin(360, getWidth() - kPad * 2 - 84 - 20);
        source_.setBounds(comboAt(rowsBaseY(), 0, comboW));
        target_.setBounds(comboAt(rowsBaseY(), 1, comboW));
        micSource_.setBounds(comboAt(micRowsBaseY(), 0, comboW));
        micTarget_.setBounds(comboAt(micRowsBaseY(), 1, comboW));
        preferred_.setBounds(comboAt(preferredY(), 0, comboW));
    }

protected:
    void paintContent(juce::Graphics& g) override
    {
        const auto inner = getLocalBounds().reduced(kPad);

        auto label = [&](int y, const char* text, bool dimmed)
        {
            g.setColour(dimmed ? palette_.textDisabled : palette_.textSecondary);
            g.setFont(fonts::body(13.0f));
            g.drawText(text, juce::Rectangle<int>(inner.getX(), y, 84, 28),
                       juce::Justification::centredLeft, false);
        };
        auto statusLine = [&](int y, const Status& st)
        {
            g.setColour(st.colour);
            g.setFont(fonts::body(11.0f));
            g.drawText(st.text, juce::Rectangle<int>(inner.getX(), y, inner.getWidth(), 16),
                       juce::Justification::topLeft, true);
        };

        // Playback bridge section.
        g.setColour(palette_.textPrimary);
        g.setFont(fonts::display(20.0f));
        g.drawText("AUDIO BRIDGE", juce::Rectangle<int>(inner.getX(), inner.getY(), 300, 28),
                   juce::Justification::centredLeft, false);
        statusLine(inner.getY() + 28, status());
        label(rowsBaseY(),      "Capture", ! bridge_.enabled);
        label(rowsBaseY() + 40, "Play to", ! bridge_.enabled);

        // Mic bridge section.
        g.setColour(palette_.textPrimary);
        g.setFont(fonts::display(15.0f));
        g.drawText("MIC BRIDGE", juce::Rectangle<int>(inner.getX(), micHeaderY(), 300, 24),
                   juce::Justification::centredLeft, false);
        statusLine(micHeaderY() + 24, micStatus());
        label(micRowsBaseY(),      "Microphone", ! mic_.enabled);
        label(micRowsBaseY() + 40, "Sends to",   ! mic_.enabled);

        // APO-path preferred output.
        label(preferredY(), "Preferred", bridge_.enabled);
        g.setColour(palette_.textTertiary);
        g.setFont(fonts::body(10.5f));
        g.drawText("Preferred output is used by the signed APO path while the Bridge is off.",
                   juce::Rectangle<int>(inner.getX(), preferredY() + 30, inner.getWidth(), 14),
                   juce::Justification::topLeft, true);
    }

private:
    struct Status { juce::String text; juce::Colour colour; };

    static bool isVirtualSink(const OutputDevice& d)
    {
        const auto n = d.name.toLowerCase();
        return n.contains("cable") || n.contains("vb-audio") || n.contains("voicemeeter")
            || n.contains("virtual");
    }

    bool haveVirtualSink() const
    {
        for (const auto& d : devices_)
            if (isVirtualSink(d)) return true;
        return false;
    }

    juce::String nameFor(const std::string& id) const
    {
        for (const auto& d : devices_)
            if (d.id == id) return d.name;
        return "unknown device";
    }

    juce::String micNameFor(const std::string& id) const
    {
        if (id.empty()) return "Default microphone";
        for (const auto& d : captureDevices_)
            if (d.id == id) return d.name;
        return "unknown microphone";
    }

    std::string defaultDeviceId() const
    {
        for (const auto& d : devices_)
            if (d.isDefault) return d.id;
        return {};
    }

    Status status() const
    {
        if (! bridge_.enabled)
            return {"Off. Turn this on to route audio through Veyra without a signed driver.",
                    palette_.textTertiary};

        if (bridge_.targetDeviceId.empty())
            return {"Pick the device you actually listen on under Play to.", palette_.warning};

        const auto resolvedSource =
            bridge_.sourceDeviceId.empty() ? defaultDeviceId() : bridge_.sourceDeviceId;
        if (resolvedSource == bridge_.targetDeviceId)
            return {"Capture and Play to must be different devices, or the sound doubles up.",
                    palette_.danger};

        if (bridge_.sourceDeviceId.empty() && ! haveVirtualSink())
            return {"Working, but for whole-system audio install the free VB-CABLE (vb-audio.com), then Refresh.",
                    palette_.warning};

        return {"Live: " + nameFor(resolvedSource) + "  to  " + nameFor(bridge_.targetDeviceId)
                    + ". Veyra keeps the capture device set as the Windows default.",
                palette_.success};
    }

    Status micStatus() const
    {
        if (! mic_.enabled)
            return {"Off. Turn this on to denoise your mic without a signed driver.",
                    palette_.textTertiary};

        if (mic_.targetDeviceId.empty())
            return {"Pick the cable that carries your clean mic under Sends to.", palette_.warning};

        if (bridge_.enabled && ! bridge_.sourceDeviceId.empty()
            && mic_.targetDeviceId == bridge_.sourceDeviceId)
            return {"That cable already carries your playback. Use a second cable (e.g. CABLE A).",
                    palette_.danger};

        return {"Live: " + micNameFor(mic_.micDeviceId) + " into " + nameFor(mic_.targetDeviceId)
                    + ". Pick that cable's output as the microphone in your apps.",
                palette_.success};
    }

    // First enable: point capture at a virtual cable when one exists, and playback
    // at the device the user is listening on right now.
    void autoPickDevices()
    {
        if (bridge_.sourceDeviceId.empty())
            for (const auto& d : devices_)
                if (isVirtualSink(d)) { bridge_.sourceDeviceId = d.id; break; }

        if (bridge_.targetDeviceId.empty())
        {
            for (const auto& d : devices_)
                if (d.isDefault && d.id != bridge_.sourceDeviceId && ! isVirtualSink(d))
                    { bridge_.targetDeviceId = d.id; break; }
            if (bridge_.targetDeviceId.empty())
                for (const auto& d : devices_)
                    if (d.id != bridge_.sourceDeviceId && ! isVirtualSink(d))
                        { bridge_.targetDeviceId = d.id; break; }
        }
    }

    // First enable of the mic bridge: default mic is right for almost everyone;
    // for the cable, prefer a virtual sink the playback bridge is NOT using.
    void autoPickMicDevices()
    {
        if (! mic_.targetDeviceId.empty())
            return;
        for (const auto& d : devices_)
            if (isVirtualSink(d) && d.id != bridge_.sourceDeviceId && d.id != bridge_.targetDeviceId)
                { mic_.targetDeviceId = d.id; break; }
    }

    // Vertical layout: title(28) + status(16) + gap(8) -> two bridge rows (40 each)
    // -> gap(6) + mic header(24) + mic status(16) + gap(6) -> two mic rows
    // -> gap(8) + preferred row + caption.
    int rowsBaseY()    const { return getLocalBounds().reduced(kPad).getY() + 52; }
    int micHeaderY()   const { return rowsBaseY() + 80 + 6; }
    int micRowsBaseY() const { return micHeaderY() + 24 + 16 + 6; }
    int preferredY()   const { return micRowsBaseY() + 80 + 8; }

    juce::Rectangle<int> comboAt(int baseY, int row, int w) const
    {
        return {getLocalBounds().reduced(kPad).getX() + 84, baseY + row * 40, juce::jmax(120, w), 28};
    }

    // Combos with a default item map id 1 -> empty string; devices sit at 2..n+1.
    std::string idForCombo(const juce::ComboBox& box, bool /*withDefaultItem*/) const
    {
        const int idx = box.getSelectedId() - 2;
        return (idx >= 0 && idx < (int) devices_.size()) ? devices_[(size_t) idx].id : std::string();
    }

    std::string idForMicCombo() const
    {
        const int idx = micSource_.getSelectedId() - 2;
        return (idx >= 0 && idx < (int) captureDevices_.size()) ? captureDevices_[(size_t) idx].id
                                                                : std::string();
    }

    void selectIds()
    {
        auto select = [](juce::ComboBox& box, const std::vector<OutputDevice>& list,
                         const std::string& id, bool hasDefaultItem)
        {
            if (id.empty())
            {
                box.setSelectedId(hasDefaultItem ? 1 : 0, juce::dontSendNotification);
                return;
            }
            int sel = hasDefaultItem ? 1 : 0;
            for (int i = 0; i < (int) list.size(); ++i)
                if (list[(size_t) i].id == id) { sel = i + 2; break; }
            box.setSelectedId(sel, juce::dontSendNotification);
        };
        enable_.setToggleState(bridge_.enabled, juce::dontSendNotification);
        select(source_, devices_, bridge_.sourceDeviceId, true);
        select(target_, devices_, bridge_.targetDeviceId, false);
        select(preferred_, devices_, bridge_.preferredOutputId, true);
        micEnable_.setToggleState(mic_.enabled, juce::dontSendNotification);
        select(micSource_, captureDevices_, mic_.micDeviceId, true);
        select(micTarget_, devices_, mic_.targetDeviceId, false);

        // Each bridge owns its routing while on; Preferred belongs to the APO path.
        source_.setEnabled(bridge_.enabled);
        target_.setEnabled(bridge_.enabled);
        preferred_.setEnabled(! bridge_.enabled);
        micSource_.setEnabled(mic_.enabled);
        micTarget_.setEnabled(mic_.enabled);

        // Offer the cable download when routing wants one and none exists.
        cable_.setVisible((bridge_.enabled || mic_.enabled) && ! haveVirtualSink());
    }

    void emit()
    {
        selectIds();
        if (onBridgeChanged)
            onBridgeChanged(bridge_);
        repaint();
    }

    void emitMic()
    {
        selectIds();
        if (onMicBridgeChanged)
            onMicBridgeChanged(mic_);
        repaint();
    }

    static constexpr int kPad = 24;

    std::vector<OutputDevice> devices_, captureDevices_;
    veyra::BridgeConfig       bridge_;
    veyra::MicBridgeConfig    mic_;
    ToggleSwitch              enable_, micEnable_;
    juce::ComboBox            source_, target_, preferred_, micSource_, micTarget_;
    juce::TextButton          refresh_, cable_;
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
    card_->onMicBridgeChanged = [this](const veyra::MicBridgeConfig& m) { if (onMicBridgeChanged) onMicBridgeChanged(m); };
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
void DevicesScreen::setMicBridge(const veyra::MicBridgeConfig& m) { card_->setMicBridge(m); }
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
