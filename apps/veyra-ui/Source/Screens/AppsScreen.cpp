#include "Screens/AppsScreen.h"

#include "Components/GlassPanel.h"
#include "Components/ToggleSwitch.h"
#include "Graphics/GlassBackground.h"
#include "Theme/Fonts.h"

#include "veyra/AppRules.h"

#include <algorithm>

namespace veyra::ui {

namespace {
juce::File rulesFile()
{
    return juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
        .getChildFile("Veyra").getChildFile("app_rules.json");
}

// Column boundaries as fractions of the row width: App | Detection | Preset |
// Volume | Auto-mute | Status | (remove). 8 edges -> 7 columns.
constexpr float kCols[8] = {0.00f, 0.26f, 0.45f, 0.67f, 0.80f, 0.89f, 0.96f, 1.00f};

juce::Rectangle<int> colRect(juce::Rectangle<int> row, int c)
{
    const int x0 = row.getX() + juce::roundToInt(kCols[c] * row.getWidth());
    const int x1 = row.getX() + juce::roundToInt(kCols[c + 1] * row.getWidth());
    return { x0, row.getY(), x1 - x0, row.getHeight() };
}
} // namespace

// One table row: App match · Detection · Preset · Volume · Auto-mute · Status.
class RuleRow : public juce::Component {
public:
    std::function<void()> onRemove;

    RuleRow()
    {
        match_.setTextToShowWhenEmpty("app (e.g. chrome)", juce::Colours::grey);
        addAndMakeVisible(match_);

        detect_.addItem("Foreground only", 1);
        detect_.addItem("Audio session", 2);
        detect_.addItem("Both", 3);
        addAndMakeVisible(detect_);

        addAndMakeVisible(preset_);

        vol_.setSliderStyle(juce::Slider::LinearBar);
        vol_.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
        vol_.setRange(0.0, 150.0, 1.0);
        vol_.setTextValueSuffix("%");
        vol_.setValue(100.0, juce::dontSendNotification);
        addAndMakeVisible(vol_);

        autoMute_.setToggleState(false, juce::dontSendNotification);
        addAndMakeVisible(autoMute_);
        status_.setToggleState(true, juce::dontSendNotification);
        addAndMakeVisible(status_);

        remove_.setButtonText("x");
        remove_.onClick = [this] { if (onRemove) onRemove(); };
        addAndMakeVisible(remove_);
    }

    void setPresets(const std::vector<veyra::Preset>& ps)
    {
        preset_.clear(juce::dontSendNotification);
        uuids_.clear();
        for (int i = 0; i < (int) ps.size(); ++i)
        {
            preset_.addItem(ps[(size_t) i].name, i + 1);
            uuids_.push_back(ps[(size_t) i].uuid);
        }
        applyPresetSelection();
    }

    void setRule(const veyra::AppRule& r)
    {
        match_.setText(r.match, juce::dontSendNotification);
        uuid_ = r.presetUuid;
        detect_.setSelectedId(r.detection == "audio" ? 2 : r.detection == "both" ? 3 : 1,
                              juce::dontSendNotification);
        vol_.setValue(juce::jlimit(0.0f, 1.5f, r.volume) * 100.0, juce::dontSendNotification);
        autoMute_.setToggleState(r.autoMute, juce::dontSendNotification);
        status_.setToggleState(r.enabled, juce::dontSendNotification);
        applyPresetSelection();
    }

    veyra::AppRule toRule() const
    {
        veyra::AppRule r;
        r.match = match_.getText().toStdString();
        const int id = preset_.getSelectedId();
        r.presetUuid = (id >= 1 && id <= (int) uuids_.size()) ? uuids_[(size_t) (id - 1)] : uuid_;
        r.enabled  = status_.getToggleState();
        r.detection = detect_.getSelectedId() == 2 ? "audio"
                      : detect_.getSelectedId() == 3 ? "both" : "foreground";
        r.volume   = (float) (vol_.getValue() / 100.0);
        r.autoMute = autoMute_.getToggleState();
        r.priority = 0;
        return r;
    }

    void setPalette(const Palette& p) { autoMute_.setPalette(p); status_.setPalette(p); }

    void resized() override
    {
        auto b = getLocalBounds();
        match_.setBounds(colRect(b, 0).reduced(4, 6));
        detect_.setBounds(colRect(b, 1).reduced(4, 7));
        preset_.setBounds(colRect(b, 2).reduced(4, 7));
        vol_.setBounds(colRect(b, 3).reduced(4, 8));
        autoMute_.setBounds(colRect(b, 4).withSizeKeepingCentre(40, 20));
        status_.setBounds(colRect(b, 5).withSizeKeepingCentre(40, 20));
        remove_.setBounds(colRect(b, 6).reduced(4, 8));
    }

private:
    void applyPresetSelection()
    {
        for (int i = 0; i < (int) uuids_.size(); ++i)
            if (uuids_[(size_t) i] == uuid_)
            { preset_.setSelectedId(i + 1, juce::dontSendNotification); return; }
    }

    juce::TextEditor         match_;
    juce::ComboBox           detect_, preset_;
    juce::Slider             vol_;
    ToggleSwitch             autoMute_, status_;
    juce::TextButton         remove_;
    std::vector<std::string> uuids_;
    std::string              uuid_;
};

// ---------------------------------------------------------------------------

class AppsScreen::Card : public GlassPanel {
public:
    std::function<void(bool)> onSwitchingChanged;

    Card()
    {
        switching_.setToggleState(true, juce::dontSendNotification);
        switching_.onClick = [this] { if (onSwitchingChanged) onSwitchingChanged(switching_.getToggleState()); };
        addAndMakeVisible(switching_);

        add_.setButtonText("+ Add App");
        add_.onClick = [this] { addRow(veyra::AppRule{}); resized(); };
        addAndMakeVisible(add_);
        save_.setButtonText("Save");
        save_.onClick = [this] { save(); };
        addAndMakeVisible(save_);

        load();
    }

    void setPresets(const std::vector<veyra::Preset>& ps)
    {
        presets_ = ps;
        for (auto& r : rows_) r->setPresets(ps);
    }

    void setSwitchingEnabled(bool on) { switching_.setToggleState(on, juce::dontSendNotification); }

    void setPalette(const Palette& p) override
    {
        GlassPanel::setPalette(p);
        switching_.setPalette(p);
        for (auto& r : rows_) r->setPalette(p);
        repaint();
    }

    void resized() override
    {
        auto c = getLocalBounds().reduced(kPad);
        auto header = c.removeFromTop(30);
        save_.setBounds(header.removeFromRight(80));
        header.removeFromRight(8);
        add_.setBounds(header.removeFromRight(110));
        header.removeFromRight(16);
        switching_.setBounds(header.removeFromRight(46).withSizeKeepingCentre(46, 22));

        c.removeFromTop(28);     // subtitle (painted)
        c.removeFromTop(26);     // column header (painted)
        for (auto& r : rows_)
        {
            r->setBounds(c.removeFromTop(40));
            c.removeFromTop(6);
        }
    }

protected:
    void paintContent(juce::Graphics& g) override
    {
        auto c = getLocalBounds().reduced(kPad);
        g.setColour(palette_.textPrimary);
        g.setFont(fonts::display(20.0f));
        g.drawText("PER-APP RULES", c.removeFromTop(30), juce::Justification::centredLeft, false);

        g.setColour(palette_.textTertiary);
        g.setFont(fonts::body(11.0f));
        g.drawText("Veyra switches presets automatically when these apps come into focus.",
                   c.removeFromTop(28), juce::Justification::topLeft, false);

        // Column header.
        auto head = c.removeFromTop(26);
        g.setColour(palette_.textSecondary);
        g.setFont(fonts::mono(10.0f, true));
        const char* titles[7] = {"APP", "DETECTION", "PRESET", "VOLUME", "AUTO-MUTE", "STATUS", ""};
        for (int i = 0; i < 6; ++i)
            g.drawText(titles[i], colRect(head, i).withTrimmedLeft(4),
                       juce::Justification::centredLeft, false);

        if (rows_.empty())
        {
            g.setColour(palette_.textTertiary);
            g.setFont(fonts::body(13.0f));
            g.drawText("No rules yet — click \"+ Add App\".", c.removeFromTop(40),
                       juce::Justification::centredLeft, false);
        }
        g.setColour(palette_.textTertiary);
        g.setFont(fonts::mono(10.0f, true));
        g.drawText(status_, getLocalBounds().reduced(kPad).removeFromBottom(16),
                   juce::Justification::centredLeft, false);
    }

private:
    void load()
    {
        rows_.clear();
        const auto f = rulesFile();
        const std::string txt = f.existsAsFile() ? f.loadFileAsString().toStdString() : std::string();
        for (const auto& r : veyra::AppRuleEngine::rulesFromJson(txt))
            addRow(r);
    }

    void addRow(const veyra::AppRule& r)
    {
        auto row = std::make_unique<RuleRow>();
        row->setPresets(presets_);
        row->setRule(r);
        row->setPalette(palette_);
        RuleRow* raw = row.get();
        row->onRemove = [this, raw]
        {
            juce::Component::SafePointer<Card> safe(this);
            juce::MessageManager::callAsync([safe, raw] { if (safe != nullptr) safe->removeRow(raw); });
        };
        addAndMakeVisible(*row);
        rows_.push_back(std::move(row));
    }

    void removeRow(RuleRow* p)
    {
        rows_.erase(std::remove_if(rows_.begin(), rows_.end(),
                                   [p](const std::unique_ptr<RuleRow>& u) { return u.get() == p; }),
                    rows_.end());
        resized();
        save();
    }

    void save()
    {
        veyra::AppRuleEngine engine;
        std::vector<veyra::AppRule> rs;
        for (auto& r : rows_)
        {
            auto rule = r->toRule();
            if (!rule.match.empty() && !rule.presetUuid.empty())
                rs.push_back(rule);
        }
        engine.setRules(rs);
        auto f = rulesFile();
        f.getParentDirectory().createDirectory();
        f.replaceWithText(juce::String(engine.toJson()));
        status_ = "Saved " + juce::String((int) rs.size()) + " rule(s).";
        repaint();
    }

    static constexpr int kPad = 24;

    std::vector<std::unique_ptr<RuleRow>> rows_;
    std::vector<veyra::Preset>            presets_;
    ToggleSwitch                          switching_;
    juce::TextButton                      add_, save_;
    juce::String                          status_;
};

// ---------------------------------------------------------------------------

AppsScreen::AppsScreen()
{
    card_ = std::make_unique<Card>();
    card_->onSwitchingChanged = [this](bool on) { if (onSwitchingChanged) onSwitchingChanged(on); };
    addAndMakeVisible(*card_);
}

AppsScreen::~AppsScreen() = default;

void AppsScreen::setPalette(const Palette& p)       { card_->setPalette(p); }
void AppsScreen::attachBackdrop(GlassBackground* b) { card_->setBackdrop(b); }
void AppsScreen::setPresets(const std::vector<veyra::Preset>& ps) { card_->setPresets(ps); }
void AppsScreen::setSwitchingEnabled(bool on)       { card_->setSwitchingEnabled(on); }
void AppsScreen::resized() { card_->setBounds(getLocalBounds().reduced(24)); }

} // namespace veyra::ui
