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
} // namespace

// One editable rule: app-match text + preset picker + enable + remove.
class RuleRow : public juce::Component {
public:
    std::function<void()> onRemove;

    RuleRow()
    {
        match_.setTextToShowWhenEmpty("app match (e.g. chrome)", juce::Colours::grey);
        addAndMakeVisible(match_);
        addAndMakeVisible(preset_);
        enabled_.setToggleState(true, juce::dontSendNotification);
        addAndMakeVisible(enabled_);
        remove_.setButtonText("Remove");
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
        applySelection();
    }

    void setRule(const veyra::AppRule& r)
    {
        match_.setText(r.match, juce::dontSendNotification);
        uuid_ = r.presetUuid;
        enabled_.setToggleState(r.enabled, juce::dontSendNotification);
        applySelection();
    }

    veyra::AppRule toRule() const
    {
        veyra::AppRule r;
        r.match = match_.getText().toStdString();
        const int id = preset_.getSelectedId();
        r.presetUuid = (id >= 1 && id <= (int) uuids_.size()) ? uuids_[(size_t) (id - 1)] : uuid_;
        r.enabled = enabled_.getToggleState();
        r.priority = 0;
        return r;
    }

    void setPalette(const Palette& p) { enabled_.setPalette(p); }

    void resized() override
    {
        auto b = getLocalBounds();
        match_.setBounds(b.removeFromLeft(170));
        b.removeFromLeft(8);
        preset_.setBounds(b.removeFromLeft(200));
        b.removeFromLeft(12);
        enabled_.setBounds(b.removeFromLeft(46).withSizeKeepingCentre(46, 24));
        b.removeFromLeft(12);
        remove_.setBounds(b.removeFromLeft(80));
    }

private:
    void applySelection()
    {
        for (int i = 0; i < (int) uuids_.size(); ++i)
            if (uuids_[(size_t) i] == uuid_)
            { preset_.setSelectedId(i + 1, juce::dontSendNotification); return; }
    }

    juce::TextEditor         match_;
    juce::ComboBox           preset_;
    ToggleSwitch             enabled_;
    juce::TextButton         remove_;
    std::vector<std::string> uuids_;
    std::string              uuid_;
};

// ---------------------------------------------------------------------------

class AppsScreen::Card : public GlassPanel {
public:
    Card()
    {
        add_.setButtonText("+ Add Rule");
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

    void setPalette(const Palette& p) override
    {
        GlassPanel::setPalette(p);
        for (auto& r : rows_) r->setPalette(p);
        repaint();
    }

    void resized() override
    {
        auto c = getLocalBounds().reduced(kPad);
        auto header = c.removeFromTop(28);
        save_.setBounds(header.removeFromRight(80));
        header.removeFromRight(8);
        add_.setBounds(header.removeFromRight(110));

        c.removeFromTop(40); // explainer (painted)
        c.removeFromTop(20); // column headers (painted)
        for (auto& r : rows_)
        {
            r->setBounds(c.removeFromTop(32));
            c.removeFromTop(8);
        }
    }

protected:
    void paintContent(juce::Graphics& g) override
    {
        auto c = getLocalBounds().reduced(kPad);
        g.setColour(palette_.textPrimary);
        g.setFont(fonts::display(20.0f));
        g.drawText("PER-APP RULES", c.removeFromTop(28), juce::Justification::centredLeft, false);

        g.setColour(palette_.textTertiary);
        g.setFont(fonts::body(11.0f));
        g.drawText("When an app takes focus, its highest-priority rule applies that preset.",
                   c.removeFromTop(16), juce::Justification::topLeft, false);
        g.drawText(status_, c.removeFromTop(16), juce::Justification::topLeft, false);

        auto cols = c.removeFromTop(20);
        g.setColour(palette_.textSecondary);
        g.setFont(fonts::mono(11.0f, true));
        g.drawText("APP", cols.withX(cols.getX()).withWidth(170), juce::Justification::centredLeft, false);
        g.drawText("PRESET", cols.withX(cols.getX() + 178).withWidth(200), juce::Justification::centredLeft, false);
        g.drawText("ON", cols.withX(cols.getX() + 390).withWidth(46), juce::Justification::centredLeft, false);

        if (rows_.empty())
        {
            g.setColour(palette_.textTertiary);
            g.setFont(fonts::body(13.0f));
            g.drawText("No rules yet — click \"+ Add Rule\".", c.removeFromTop(40),
                       juce::Justification::centredLeft, false);
        }
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
    juce::TextButton                      add_, save_;
    juce::String                          status_;
};

// ---------------------------------------------------------------------------

AppsScreen::AppsScreen()
{
    card_ = std::make_unique<Card>();
    addAndMakeVisible(*card_);
}

AppsScreen::~AppsScreen() = default;

void AppsScreen::setPalette(const Palette& p)       { card_->setPalette(p); }
void AppsScreen::attachBackdrop(GlassBackground* b) { card_->setBackdrop(b); }
void AppsScreen::setPresets(const std::vector<veyra::Preset>& ps) { card_->setPresets(ps); }
void AppsScreen::resized() { card_->setBounds(getLocalBounds().reduced(24)); }

} // namespace veyra::ui
