#include "Screens/PresetsScreen.h"

#include "Components/GlassPanel.h"
#include "Graphics/GlassBackground.h"
#include "Theme/Fonts.h"

namespace veyra::ui {

namespace {
constexpr int kPad = 24;
constexpr int kTileW = 230;
constexpr int kTileH = 92;
constexpr int kGap = 16;
} // namespace

// ---------------------------------------------------------------------------
// Tile: one preset card; click to apply, active gets an accent ring.
// ---------------------------------------------------------------------------
class PresetsScreen::Tile : public GlassPanel {
public:
    explicit Tile(veyra::Preset p) : preset_(std::move(p)) { setElevated(true); }

    void setActive(bool a) { active_ = a; repaint(); }
    const std::string& uuid() const { return preset_.uuid; }

    std::function<void()> onClick;

    void mouseEnter(const juce::MouseEvent&) override { hover_ = true; repaint(); }
    void mouseExit(const juce::MouseEvent&) override { hover_ = false; repaint(); }
    void mouseDown(const juce::MouseEvent&) override { if (onClick) onClick(); }

protected:
    void paintContent(juce::Graphics& g) override
    {
        auto b = getLocalBounds().reduced(14);

        g.setColour(palette_.textPrimary);
        g.setFont(fonts::display(16.0f));
        g.drawText(preset_.name, b.removeFromTop(22), juce::Justification::topLeft, false);

        g.setColour(palette_.textTertiary);
        g.setFont(fonts::body(12.0f));
        g.drawText(preset_.category, b.removeFromTop(18), juce::Justification::topLeft, false);

        // Tag pill (built-in vs user).
        const bool builtin = preset_.builtIn;
        auto tag = juce::Rectangle<int>(b.getX(), getHeight() - 26, 64, 16);
        g.setColour(builtin ? palette_.accentSecondaryDim : palette_.bgGlassHover);
        g.fillRoundedRectangle(tag.toFloat(), 8.0f);
        g.setColour(builtin ? palette_.accentSecondary : palette_.textSecondary);
        g.setFont(fonts::mono(9.0f, true));
        g.drawText(builtin ? "BUILT-IN" : "USER", tag, juce::Justification::centred, false);

        if (active_)
        {
            g.setColour(palette_.accentPrimary);
            g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(1.5f), 16.0f, 2.0f);
            // "Active" check dot, top-right.
            g.fillEllipse((float) getWidth() - 22.0f, 14.0f, 8.0f, 8.0f);
        }
        else if (hover_)
        {
            g.setColour(palette_.strokeHover);
            g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(0.5f), 16.0f, 1.0f);
        }
    }

private:
    veyra::Preset preset_;
    bool active_ = false;
    bool hover_  = false;
};

// ---------------------------------------------------------------------------
// Grid: the scrollable viewed component holding the tiles.
// ---------------------------------------------------------------------------
class PresetsScreen::Grid : public juce::Component {
public:
    std::function<void(juce::String)> onApply;

    void setBackdrop(GlassBackground* b) { backdrop_ = b; for (auto& t : tiles_) t->setBackdrop(b); }
    void setPalette(const Palette& p) { palette_ = p; for (auto& t : tiles_) t->setPalette(p); }

    void setPresets(const std::vector<veyra::Preset>& presets, const juce::String& active)
    {
        tiles_.clear();
        for (const auto& p : presets)
        {
            auto t = std::make_unique<Tile>(p);
            t->setBackdrop(backdrop_);
            t->setPalette(palette_);
            t->setActive(juce::String(p.uuid) == active);
            const juce::String uuid(p.uuid);
            t->onClick = [this, uuid] { if (onApply) onApply(uuid); };
            addAndMakeVisible(*t);
            tiles_.push_back(std::move(t));
        }
        resized();
    }

    void setListMode(bool m) { listMode_ = m; resized(); }

    int columnsFor(int width) const
    {
        return listMode_ ? 1 : juce::jmax(1, (width + kGap) / (kTileW + kGap));
    }

    int tileHeight() const { return listMode_ ? 54 : kTileH; }

    int preferredHeight(int width) const
    {
        if (tiles_.empty())
            return 0;
        const int cols = columnsFor(width);
        const int rows = ((int) tiles_.size() + cols - 1) / cols;
        return rows * tileHeight() + (rows - 1) * kGap;
    }

    void resized() override
    {
        const int cols = columnsFor(getWidth());
        const int th = tileHeight();
        const int tileW = (getWidth() - kGap * (cols - 1)) / juce::jmax(1, cols);
        for (int i = 0; i < (int) tiles_.size(); ++i)
        {
            const int col = i % cols, row = i / cols;
            tiles_[(size_t) i]->setBounds(col * (tileW + kGap), row * (th + kGap), tileW, th);
        }
    }

private:
    std::vector<std::unique_ptr<Tile>> tiles_;
    GlassBackground* backdrop_ = nullptr;
    Palette palette_ = paletteForTheme("midnight");
    bool listMode_ = false;
};

// ---------------------------------------------------------------------------
// PresetsScreen
// ---------------------------------------------------------------------------
PresetsScreen::PresetsScreen()
{
    grid_ = std::make_unique<Grid>();
    grid_->onApply = [this](juce::String uuid) { if (onApply) onApply(uuid); };

    viewport_.setViewedComponent(grid_.get(), false);
    viewport_.setScrollBarsShown(true, false);
    addAndMakeVisible(viewport_);

    saveBtn_.onClick   = [this] { promptSaveCurrent(); };
    importBtn_.onClick = [this] { if (onImport) onImport(); };
    exportBtn_.onClick = [this] { if (onExport && activeUuid_.isNotEmpty()) onExport(activeUuid_); };
    deleteBtn_.onClick = [this] { if (onDelete && activeUuid_.isNotEmpty()) onDelete(activeUuid_); };
    for (auto* b : {&saveBtn_, &importBtn_, &exportBtn_, &deleteBtn_})
        addAndMakeVisible(b);

    search_.setTextToShowWhenEmpty("Search presets...", juce::Colours::grey);
    search_.onTextChange = [this] { applyFilter(); resized(); };
    addAndMakeVisible(search_);

    viewToggle_.setItems({"Grid", "List"});
    viewToggle_.onChange = [this](int i) { grid_->setListMode(i == 1); resized(); };
    addAndMakeVisible(viewToggle_);
}

PresetsScreen::~PresetsScreen() = default;

void PresetsScreen::setPalette(const Palette& p)
{
    palette_ = p;
    grid_->setPalette(p);
    viewToggle_.setPalette(p);
    repaint();
}

void PresetsScreen::attachBackdrop(GlassBackground* bg)
{
    backdrop_ = bg;
    grid_->setBackdrop(bg);
}

void PresetsScreen::setPresets(std::vector<veyra::Preset> presets, juce::String activeUuid)
{
    activeUuid_ = activeUuid;
    allPresets_ = std::move(presets);

    // Build the category column: All + each built-in category (in first-seen
    // order) + Custom (if any user presets exist).
    categories_ = juce::StringArray{"All Presets"};
    bool anyCustom = false;
    for (const auto& p : allPresets_)
    {
        if (!p.builtIn) { anyCustom = true; continue; }
        const juce::String cat(p.category);
        if (cat.isNotEmpty() && !categories_.contains(cat))
            categories_.add(cat);
    }
    if (anyCustom)
        categories_.add("Custom");
    selectedCat_ = juce::jlimit(0, categories_.size() - 1, selectedCat_);

    applyFilter();
    resized();
}

void PresetsScreen::applyFilter()
{
    const juce::String cat = categories_[selectedCat_];
    const juce::String q = search_.getText().trim();
    std::vector<veyra::Preset> filtered;
    for (const auto& p : allPresets_)
    {
        bool catOk = (selectedCat_ == 0)
                     || (cat == "Custom" ? !p.builtIn : juce::String(p.category) == cat);
        bool qOk = q.isEmpty() || juce::String(p.name).containsIgnoreCase(q);
        if (catOk && qOk)
            filtered.push_back(p);
    }
    grid_->setPresets(filtered, activeUuid_);
}

void PresetsScreen::selectCategory(int i)
{
    selectedCat_ = juce::jlimit(0, categories_.size() - 1, i);
    applyFilter();
    resized();
    repaint();
}

juce::Rectangle<int> PresetsScreen::categoryColumn() const
{
    auto content = getLocalBounds().reduced(kPad);
    content.removeFromTop(34 + 12); // title + gap
    return content.removeFromLeft(170);
}

juce::Rectangle<int> PresetsScreen::categoryItemBounds(int i) const
{
    auto col = categoryColumn();
    return { col.getX(), col.getY() + i * 34, col.getWidth() - 8, 30 };
}

void PresetsScreen::mouseDown(const juce::MouseEvent& e)
{
    for (int i = 0; i < categories_.size(); ++i)
        if (categoryItemBounds(i).contains(e.getPosition()))
        {
            selectCategory(i);
            return;
        }
}

void PresetsScreen::promptSaveCurrent()
{
    auto* aw = new juce::AlertWindow("Save Preset", "Name your preset:",
                                     juce::MessageBoxIconType::NoIcon);
    aw->addTextEditor("name", "My Preset");
    aw->addButton("Save", 1, juce::KeyPress(juce::KeyPress::returnKey));
    aw->addButton("Cancel", 0, juce::KeyPress(juce::KeyPress::escapeKey));
    aw->enterModalState(true, juce::ModalCallbackFunction::create([this, aw](int r)
    {
        if (r == 1)
        {
            const auto name = aw->getTextEditorContents("name").trim();
            if (onSaveCurrent && name.isNotEmpty())
                onSaveCurrent(name);
        }
        delete aw;
    }), false);
}

void PresetsScreen::resized()
{
    auto b = getLocalBounds().reduced(kPad);
    b.removeFromTop(34 + 12);     // title (painted)
    b.removeFromLeft(170);        // category column (painted)
    b.removeFromLeft(20);         // gutter

    auto header = b.removeFromTop(40);
    int bw = 92, gap = 8;
    auto place = [&](juce::TextButton& btn)
    {
        btn.setBounds(header.removeFromRight(bw).reduced(0, 4));
        header.removeFromRight(gap);
    };
    place(deleteBtn_);
    place(exportBtn_);
    place(importBtn_);
    place(saveBtn_);
    header.removeFromRight(12);
    viewToggle_.setBounds(header.removeFromRight(140).reduced(0, 5));
    header.removeFromRight(12);
    search_.setBounds(header.removeFromLeft(juce::jmin(260, header.getWidth())).reduced(0, 5));

    b.removeFromTop(12);
    viewport_.setBounds(b);
    const int w = viewport_.getMaximumVisibleWidth();
    grid_->setBounds(0, 0, w, grid_->preferredHeight(w));
}

void PresetsScreen::paint(juce::Graphics& g)
{
    g.setColour(palette_.textPrimary);
    g.setFont(fonts::display(24.0f));
    g.drawText("PRESETS", getLocalBounds().reduced(kPad).removeFromTop(34),
               juce::Justification::centredLeft, false);

    // Category column.
    for (int i = 0; i < categories_.size(); ++i)
    {
        const auto r = categoryItemBounds(i);
        if (i == selectedCat_)
        {
            auto rf = r.toFloat();
            g.setColour(palette_.bgGlassActive);
            g.fillRoundedRectangle(rf, 8.0f);
            g.setColour(palette_.accentPrimary);
            g.fillRoundedRectangle(rf.removeFromLeft(3.0f), 1.5f);
            g.setColour(palette_.textPrimary);
        }
        else
        {
            g.setColour(palette_.textSecondary);
        }
        g.setFont(fonts::body(13.0f, i == selectedCat_));
        g.drawText(categories_[i], r.withTrimmedLeft(14), juce::Justification::centredLeft, false);
    }
}

} // namespace veyra::ui
