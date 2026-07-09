#include "Screens/AppsScreen.h"

#include "AppIndex.h"

#include "Components/GlassPanel.h"
#include "Components/ToggleSwitch.h"
#include "Graphics/GlassBackground.h"
#include "Theme/Fonts.h"

#include "veyra/AppRules.h"
#include "veyra/Paths.h"

#include <algorithm>
#include <set>

namespace veyra::ui {

namespace {
juce::File rulesFile()
{
    return juce::File(juce::String(
        (veyra::paths::appDataDir() / "app_rules.json").string()));
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

// The APP cell: a click-to-pick control (icon + name + chevron) — no manual
// typing. Clicking fires onClick; the Card opens the installed/running app picker
// and writes the chosen app back via set().
class AppPickerCell : public juce::Component {
public:
    std::function<void()> onClick;

    void set(const juce::String& name, const juce::String& match, const juce::Image& icon)
    {
        name_ = name; match_ = match; icon_ = icon; repaint();
    }
    // When restoring a saved rule we only have the match string (no friendly name
    // or icon); show the match as the label and a default icon.
    void setFromMatch(const juce::String& match)
    {
        match_ = match; name_ = match; icon_ = juce::Image(); repaint();
    }
    const juce::String& match() const { return match_; }

    void setPalette(const Palette& p) { palette_ = p; repaint(); }

    void mouseDown(const juce::MouseEvent&) override { if (onClick) onClick(); }
    void mouseEnter(const juce::MouseEvent&) override { hover_ = true; repaint(); }
    void mouseExit(const juce::MouseEvent&) override { hover_ = false; repaint(); }

    void paint(juce::Graphics& g) override
    {
        auto b = getLocalBounds().toFloat().reduced(0.5f);
        g.setColour(palette_.bgInput.withAlpha(0.5f));
        g.fillRoundedRectangle(b, 6.0f);
        g.setColour(hover_ ? palette_.strokeActive : palette_.strokeDefault);
        g.drawRoundedRectangle(b, 6.0f, 1.0f);

        auto r = getLocalBounds().reduced(8, 0);
        const juce::Image& ic = icon_.isValid() ? icon_ : defaultAppIcon();
        if (ic.isValid())
            g.drawImage(ic, r.removeFromLeft(20).withSizeKeepingCentre(18, 18).toFloat(),
                        juce::RectanglePlacement::centred);
        r.removeFromLeft(6);

        auto chev = r.removeFromRight(16);
        juce::Path p;
        const float cx = (float) chev.getCentreX(), cy = (float) chev.getCentreY();
        p.startNewSubPath(cx - 3.0f, cy - 2.0f); p.lineTo(cx, cy + 2.0f); p.lineTo(cx + 3.0f, cy - 2.0f);
        g.setColour(palette_.textSecondary);
        g.strokePath(p, juce::PathStrokeType(1.3f));

        g.setColour(match_.isEmpty() ? palette_.textTertiary : palette_.textPrimary);
        g.setFont(fonts::body(13.0f));
        g.drawText(match_.isEmpty() ? "Choose app..." : name_, r, juce::Justification::centredLeft, true);
    }

private:
    juce::String name_, match_;
    juce::Image  icon_;
    bool         hover_ = false;
    Palette      palette_ = paletteForTheme("midnight");
};

// One table row: App match · Detection · Preset · Volume · Auto-mute · Status.
class RuleRow : public juce::Component {
public:
    std::function<void()> onRemove;
    std::function<void()> onPickApp; // Card shows the picker, then calls setApp()

    RuleRow()
    {
        appCell_.onClick = [this] { if (onPickApp) onPickApp(); };
        addAndMakeVisible(appCell_);

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

    void setApp(const InstalledApp& a) { appCell_.set(a.name, a.match, a.icon); }

    void setRule(const veyra::AppRule& r)
    {
        appCell_.setFromMatch(juce::String(r.match));
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
        r.match = appCell_.match().toStdString();
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

    void setPalette(const Palette& p)
    {
        autoMute_.setPalette(p);
        status_.setPalette(p);
        appCell_.setPalette(p);
        // Blend the cells into the dark glass instead of default-JUCE grey.
        for (juce::ComboBox* cb : {&detect_, &preset_})
        {
            cb->setColour(juce::ComboBox::backgroundColourId, p.bgInput.withAlpha(0.5f));
            cb->setColour(juce::ComboBox::textColourId, p.textPrimary);
            cb->setColour(juce::ComboBox::outlineColourId, juce::Colours::transparentBlack);
            cb->setColour(juce::ComboBox::arrowColourId, p.textSecondary);
        }
        vol_.setColour(juce::Slider::trackColourId, p.accentPrimary.withAlpha(0.6f));
        vol_.setColour(juce::Slider::backgroundColourId, p.bgInput.withAlpha(0.5f));
        vol_.setColour(juce::Slider::textBoxTextColourId, p.textPrimary);
    }

    void resized() override
    {
        auto b = getLocalBounds();
        appCell_.setBounds(colRect(b, 0).reduced(4, 6));
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

    AppPickerCell            appCell_;
    juce::ComboBox           detect_, preset_;
    juce::Slider             vol_;
    ToggleSwitch             autoMute_, status_;
    juce::TextButton         remove_;
    std::vector<std::string> uuids_;
    std::string              uuid_;
};

// ---------------------------------------------------------------------------
// Searchable app picker shown in a callout: a filter field over an icon'd list of
// running + installed apps. Typing filters live; Enter on no match creates a
// custom rule from the typed text (so manual entry needs no separate dialog).
// ---------------------------------------------------------------------------
class AppPickerPopup : public juce::Component,
                       private juce::ListBoxModel,
                       private juce::TextEditor::Listener
{
public:
    std::function<void(const InstalledApp&)> onChosen;

    AppPickerPopup(std::vector<InstalledApp> apps, Palette p)
        : all_(std::move(apps)), palette_(std::move(p))
    {
        search_.setTextToShowWhenEmpty("Search apps...", palette_.textTertiary);
        search_.setColour(juce::TextEditor::backgroundColourId, palette_.bgInput.withAlpha(0.6f));
        search_.setColour(juce::TextEditor::outlineColourId, palette_.strokeDefault);
        search_.setColour(juce::TextEditor::focusedOutlineColourId, palette_.strokeActive);
        search_.setColour(juce::TextEditor::textColourId, palette_.textPrimary);
        search_.addListener(this);
        addAndMakeVisible(search_);

        list_.setModel(this);
        list_.setRowHeight(34);
        list_.setColour(juce::ListBox::backgroundColourId, juce::Colours::transparentBlack);
        addAndMakeVisible(list_);

        filter({});
        setSize(300, 380);
    }

    void resized() override
    {
        auto b = getLocalBounds().reduced(8);
        search_.setBounds(b.removeFromTop(30));
        b.removeFromTop(6);
        list_.setBounds(b);
    }

    void paint(juce::Graphics& g) override
    {
        g.setColour(palette_.bgGlassElevated);
        g.fillRoundedRectangle(getLocalBounds().toFloat(), 10.0f);
        g.setColour(palette_.strokeDefault);
        g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(0.5f), 10.0f, 1.0f);
    }

    void visibilityChanged() override { if (isShowing()) search_.grabKeyboardFocus(); }

    // ListBoxModel
    int getNumRows() override { return (int) filtered_.size(); }
    void paintListBoxItem(int row, juce::Graphics& g, int w, int h, bool selected) override
    {
        if (row < 0 || row >= (int) filtered_.size()) return;
        const auto& a = all_[(size_t) filtered_[(size_t) row]];
        if (selected)
        {
            g.setColour(palette_.bgGlassHover);
            g.fillRoundedRectangle(2.0f, 2.0f, (float) w - 4.0f, (float) h - 4.0f, 6.0f);
        }
        const juce::Image& ic = a.icon.isValid() ? a.icon : defaultAppIcon();
        if (ic.isValid())
            g.drawImage(ic, juce::Rectangle<float>(8.0f, (h - 20.0f) / 2.0f, 20.0f, 20.0f),
                        juce::RectanglePlacement::centred);
        g.setColour(palette_.textPrimary);
        g.setFont(fonts::body(13.0f));
        g.drawText(a.name, 36, 0, w - 44, h, juce::Justification::centredLeft, true);
    }
    void listBoxItemClicked(int row, const juce::MouseEvent&) override { choose(row); }
    void returnKeyPressed(int row) override { choose(row); }

    // TextEditor::Listener
    void textEditorTextChanged(juce::TextEditor&) override { filter(search_.getText().trim()); }
    void textEditorReturnKeyPressed(juce::TextEditor&) override
    {
        if (! filtered_.empty()) { choose(0); return; }
        const auto t = search_.getText().trim();
        if (t.isEmpty()) return;
        InstalledApp a;
        a.name  = t;
        a.match = t.toLowerCase().upToLastOccurrenceOf(".exe", false, true);
        emit(a);
    }
    void textEditorEscapeKeyPressed(juce::TextEditor&) override { dismiss(); }

private:
    void filter(const juce::String& q)
    {
        filtered_.clear();
        for (int i = 0; i < (int) all_.size(); ++i)
            if (q.isEmpty() || all_[(size_t) i].name.containsIgnoreCase(q)
                || all_[(size_t) i].match.containsIgnoreCase(q))
                filtered_.push_back(i);
        list_.updateContent();
        list_.repaint();
    }
    void choose(int row)
    {
        if (row < 0 || row >= (int) filtered_.size()) return;
        emit(all_[(size_t) filtered_[(size_t) row]]);
    }
    void emit(const InstalledApp& a)
    {
        if (onChosen) onChosen(a);
        dismiss();
    }
    void dismiss()
    {
        if (auto* cb = findParentComponentOfClass<juce::CallOutBox>())
            cb->dismiss();
    }

    std::vector<InstalledApp> all_;
    std::vector<int>          filtered_;
    juce::TextEditor          search_;
    juce::ListBox             list_{"apps", nullptr};
    Palette                   palette_;
};

// ---------------------------------------------------------------------------

class AppsScreen::Card : public GlassPanel {
public:
    std::function<void(bool)>              onSwitchingChanged;
    std::function<void(const std::string&)> onRulesSaved;

    Card()
    {
        switching_.setToggleState(true, juce::dontSendNotification);
        switching_.onClick = [this] { if (onSwitchingChanged) onSwitchingChanged(switching_.getToggleState()); };
        addAndMakeVisible(switching_);

        add_.setButtonText("+ Add App");
        add_.onClick = [this]
        {
            addRow(veyra::AppRule{});
            resized();
            if (! rows_.empty() && rows_.back()->onPickApp)
                rows_.back()->onPickApp(); // open the picker straight away for the new row
        };
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
        g.setColour(palette_.strokeDefault);
        g.fillRect(head.getX(), head.getBottom() - 1, head.getWidth(), 1); // header underline

        // Faint separators between rows for a clean table read.
        for (size_t i = 1; i < rows_.size(); ++i)
        {
            const auto rb = rows_[i]->getBounds();
            g.setColour(palette_.strokeDefault.withAlpha(0.5f));
            g.fillRect(rb.getX(), rb.getY() - 3, rb.getWidth(), 1);
        }

        if (rows_.empty())
        {
            g.setColour(palette_.textTertiary);
            g.setFont(fonts::body(13.0f));
            g.drawText("No rules yet. Click \"+ Add App\".", c.removeFromTop(40),
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

    // Scan installed apps + running processes once (icons extracted locally from
    // the EXEs); cached so the menu opens instantly thereafter.
    void ensureIndexed()
    {
        if (indexed_) return;
        installed_ = scanInstalledApps();
        running_   = scanRunningProcesses();
        indexed_   = true;
    }

    // Searchable picker: a callout with a filter field over an icon'd list of
    // running + installed apps (de-duplicated). Typing filters; Enter on no match
    // creates a custom rule from the text — so manual entry needs no separate step.
    void showAppPicker(std::function<void(const InstalledApp&)> onChosen)
    {
        ensureIndexed();

        std::vector<InstalledApp> merged = running_;
        std::set<std::string> seen;
        for (const auto& a : running_)
            seen.insert(a.match.toStdString());
        for (const auto& a : installed_)
            if (seen.insert(a.match.toStdString()).second)
                merged.push_back(a);

        auto popup = std::make_unique<AppPickerPopup>(std::move(merged), palette_);
        popup->onChosen = std::move(onChosen);
        juce::CallOutBox::launchAsynchronously(std::move(popup), add_.getScreenBounds(), nullptr);
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
        row->onPickApp = [this, raw]
        {
            juce::Component::SafePointer<Card> safe(this);
            showAppPicker([safe, raw](const InstalledApp& a)
            {
                if (safe == nullptr) return;
                const bool alive = std::any_of(safe->rows_.begin(), safe->rows_.end(),
                                               [raw](const std::unique_ptr<RuleRow>& u) { return u.get() == raw; });
                if (alive) { raw->setApp(a); safe->save(); }
            });
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
        const std::string rulesJson = engine.toJson();
        auto f = rulesFile();
        f.getParentDirectory().createDirectory();
        f.replaceWithText(juce::String(rulesJson));
        if (onRulesSaved) onRulesSaved(rulesJson); // sync service in-memory state
        status_ = "Saved " + juce::String((int) rs.size()) + " rule(s).";
        repaint();
    }

    static constexpr int kPad = 24;

    std::vector<std::unique_ptr<RuleRow>> rows_;
    std::vector<veyra::Preset>            presets_;
    ToggleSwitch                          switching_;
    juce::TextButton                      add_, save_;
    juce::String                          status_;
    std::vector<InstalledApp>             installed_, running_;
    bool                                  indexed_ = false;
};

// ---------------------------------------------------------------------------

AppsScreen::AppsScreen()
{
    card_ = std::make_unique<Card>();
    card_->onSwitchingChanged = [this](bool on) { if (onSwitchingChanged) onSwitchingChanged(on); };
    card_->onRulesSaved = [this](const std::string& j) { if (onRulesSaved) onRulesSaved(j); };
    addAndMakeVisible(*card_);
}

AppsScreen::~AppsScreen() = default;

void AppsScreen::setPalette(const Palette& p)       { card_->setPalette(p); }
void AppsScreen::attachBackdrop(GlassBackground* b) { card_->setBackdrop(b); }
void AppsScreen::setPresets(const std::vector<veyra::Preset>& ps) { card_->setPresets(ps); }
void AppsScreen::setSwitchingEnabled(bool on)       { card_->setSwitchingEnabled(on); }
void AppsScreen::resized() { card_->setBounds(getLocalBounds().reduced(24)); }

} // namespace veyra::ui
