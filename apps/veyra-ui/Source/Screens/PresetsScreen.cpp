#include "Screens/PresetsScreen.h"

#include "Components/GlassPanel.h"
#include "Graphics/GlassBackground.h"
#include "Theme/Fonts.h"

#include <algorithm>
#include <cmath>

namespace veyra::ui {

namespace {
constexpr int kPad = 24;
constexpr int kTileW = 230;
constexpr int kTileH = 92;
constexpr int kGap = 16;

// Curated "Popular Presets" — top 10 built-ins, in display order.
const std::vector<std::string> kPopular = {
    "v-studio-flat",       // Flat
    "v-soundmax",          // SoundMax (safe max loudness)
    "v-bass-monster",      // Bass Boost
    "v-vocal-boost",       // Vocal Clarity
    "v-cinema",            // Movie Cinema
    "v-fps-competitive",   // Gaming Competitive
    "v-apex-awareness",    // Gaming Immersive
    "v-rock-arena",        // Music Wide
    "v-night-listening",   // Night Mode
    "v-podcast-voice",     // Podcast Voice
};
int popularRank(const std::string& uuid)
{
    const auto it = std::find(kPopular.begin(), kPopular.end(), uuid);
    return it == kPopular.end() ? (int) kPopular.size() : (int) std::distance(kPopular.begin(), it);
}

juce::File veyraStateFile(const char* name)
{
    return juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
        .getChildFile("Veyra").getChildFile(name);
}
} // namespace

// ---------------------------------------------------------------------------
// Tile: one preset card; click to apply, active gets an accent ring.
// ---------------------------------------------------------------------------
class PresetsScreen::Tile : public GlassPanel {
public:
    explicit Tile(veyra::Preset p) : preset_(std::move(p)) { setElevated(true); }

    void setActive(bool a) { active_ = a; repaint(); }
    void setFavorite(bool f) { favorite_ = f; repaint(); }
    const std::string& uuid() const { return preset_.uuid; }

    std::function<void()> onClick;
    std::function<void()> onToggleFavorite;

    void mouseEnter(const juce::MouseEvent&) override { hover_ = true; repaint(); }
    void mouseExit(const juce::MouseEvent&) override { hover_ = false; repaint(); }
    void mouseDown(const juce::MouseEvent& e) override
    {
        if (starBounds().contains(e.getPosition())) { if (onToggleFavorite) onToggleFavorite(); return; }
        if (onClick) onClick();
    }

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

        // Tag pill: BUILT-IN is white; USER keeps the glass look. Bottom padding
        // (14) matches the content inset so the badge sits intentionally within the
        // card; the pill radius is half its height for a true capsule.
        const bool builtin = preset_.builtIn;
        const int tagH = 18, tagW = builtin ? 74 : 50;
        auto tag = juce::Rectangle<int>(14, getHeight() - 14 - tagH, tagW, tagH);
        g.setColour(builtin ? juce::Colours::white : palette_.bgGlassHover);
        g.fillRoundedRectangle(tag.toFloat(), tagH * 0.5f);
        g.setColour(builtin ? palette_.bgCanvas : palette_.textSecondary);
        g.setFont(fonts::mono(9.5f, true));
        g.drawText(builtin ? "BUILT-IN" : "USER", tag, juce::Justification::centred, false);

        // Hover/selection rings derive from the SAME radius as the glass base
        // (radius_), drawn concentric to its border so the card reads as one
        // object with clean, identical corners — never a second mismatched edge.
        if (active_)
        {
            g.setColour(palette_.accentPrimary);
            g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(1.0f), radius_ - 0.5f, 2.0f);
        }
        else if (hover_)
        {
            g.setColour(palette_.strokeHover);
            g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(0.5f), radius_, 1.0f);
        }

        // Favorite star (top-right): filled when favorited, outline otherwise.
        const auto sb = starBounds().toFloat();
        juce::Path star;
        const float cx = sb.getCentreX(), cy = sb.getCentreY();
        const float ro = sb.getWidth() * 0.46f, ri = ro * 0.45f;
        for (int i = 0; i < 10; ++i)
        {
            const float a = -juce::MathConstants<float>::halfPi + (float) i * juce::MathConstants<float>::pi / 5.0f;
            const float r = (i % 2 == 0) ? ro : ri;
            const juce::Point<float> pt(cx + std::cos(a) * r, cy + std::sin(a) * r);
            if (i == 0) star.startNewSubPath(pt); else star.lineTo(pt);
        }
        star.closeSubPath();
        if (favorite_) { g.setColour(palette_.warning); g.fillPath(star); }
        else           { g.setColour(hover_ ? palette_.textSecondary : palette_.textTertiary);
                         g.strokePath(star, juce::PathStrokeType(1.0f)); }
    }

private:
    juce::Rectangle<int> starBounds() const { return {getWidth() - 30, 10, 20, 20}; }

    veyra::Preset preset_;
    bool active_   = false;
    bool hover_    = false;
    bool favorite_ = false;
};

// ---------------------------------------------------------------------------
// Grid: the scrollable viewed component holding the tiles.
// ---------------------------------------------------------------------------
class PresetsScreen::Grid : public juce::Component {
public:
    std::function<void(juce::String)> onApply;
    std::function<void(juce::String)> onToggleFavorite;

    void setBackdrop(GlassBackground* b) { backdrop_ = b; for (auto& t : tiles_) t->setBackdrop(b); }
    void setPalette(const Palette& p) { palette_ = p; for (auto& t : tiles_) t->setPalette(p); }
    void setFavorites(std::set<std::string> f) { favorites_ = std::move(f); }

    void setPresets(const std::vector<veyra::Preset>& presets, const juce::String& active)
    {
        tiles_.clear();
        for (const auto& p : presets)
        {
            auto t = std::make_unique<Tile>(p);
            t->setBackdrop(backdrop_);
            t->setPalette(palette_);
            t->setActive(juce::String(p.uuid) == active);
            t->setFavorite(favorites_.count(p.uuid) > 0);
            const juce::String uuid(p.uuid);
            t->onClick = [this, uuid] { if (onApply) onApply(uuid); };
            t->onToggleFavorite = [this, uuid] { if (onToggleFavorite) onToggleFavorite(uuid); };
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
    std::set<std::string> favorites_;
    GlassBackground* backdrop_ = nullptr;
    Palette palette_ = paletteForTheme("midnight");
    bool listMode_ = false;
};

// ---------------------------------------------------------------------------
// PresetsScreen
// ---------------------------------------------------------------------------
PresetsScreen::PresetsScreen()
{
    loadFavoritesAndRecent();

    grid_ = std::make_unique<Grid>();
    grid_->onApply = [this](juce::String uuid) { recordRecent(uuid); if (onApply) onApply(uuid); };
    grid_->onToggleFavorite = [this](juce::String uuid) { toggleFavorite(uuid); };

    viewport_.setViewedComponent(grid_.get(), false);
    viewport_.setScrollBarsShown(true, false);
    addAndMakeVisible(viewport_);

    saveBtn_.onClick   = [this] { promptSaveCurrent(); };
    importBtn_.onClick = [this] { if (onImport) onImport(); };
    exportBtn_.onClick = [this] { if (onExport && activeUuid_.isNotEmpty()) onExport(activeUuid_); };
    dupBtn_.onClick    = [this] { if (onDuplicate && activeUuid_.isNotEmpty()) onDuplicate(activeUuid_); };
    deleteBtn_.onClick = [this] { if (onDelete && activeUuid_.isNotEmpty()) onDelete(activeUuid_); };
    for (auto* b : {&saveBtn_, &importBtn_, &exportBtn_, &dupBtn_, &deleteBtn_})
        addAndMakeVisible(b);

    // Oval (pill) search field: transparent editor over a painted pill.
    search_.setTextToShowWhenEmpty("Search presets...", juce::Colours::grey);
    search_.setColour(juce::TextEditor::backgroundColourId, juce::Colours::transparentBlack);
    search_.setColour(juce::TextEditor::outlineColourId, juce::Colours::transparentBlack);
    search_.setColour(juce::TextEditor::focusedOutlineColourId, juce::Colours::transparentBlack);
    search_.setIndents(16, 6);
    search_.onTextChange = [this] { applyFilter(); resized(); };
    addAndMakeVisible(search_);

    viewToggle_.setItems({"Grid", "List"});
    viewToggle_.onChange = [this](int i) { grid_->setListMode(i == 1); resized(); };
    addAndMakeVisible(viewToggle_);

    sort_.setItems({"A-Z", "Category"});
    sort_.onChange = [this](int i) { sortMode_ = i; applyFilter(); resized(); };
    addAndMakeVisible(sort_);
}

PresetsScreen::~PresetsScreen() = default;

void PresetsScreen::setPalette(const Palette& p)
{
    palette_ = p;
    grid_->setPalette(p);
    viewToggle_.setPalette(p);
    sort_.setPalette(p);
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

    // Build the category column: All + Favorites + Recently Used + each built-in
    // category (in first-seen order) + Custom (if any user presets exist).
    categories_ = juce::StringArray{"All Presets", "Popular", "Favorites", "Recently Used"};
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
    const bool recentView = (cat == "Recently Used");
    const bool popularView = (cat == "Popular");
    std::vector<veyra::Preset> filtered;
    for (const auto& p : allPresets_)
    {
        bool catOk;
        if (cat == "All Presets")        catOk = true;
        else if (popularView)            catOk = popularRank(p.uuid) < (int) kPopular.size();
        else if (cat == "Favorites")     catOk = favorites_.count(p.uuid) > 0;
        else if (recentView)             catOk = std::find(recent_.begin(), recent_.end(), p.uuid) != recent_.end();
        else if (cat == "Custom")        catOk = !p.builtIn;
        else                             catOk = juce::String(p.category) == cat;
        bool qOk = q.isEmpty() || juce::String(p.name).containsIgnoreCase(q);
        if (catOk && qOk)
            filtered.push_back(p);
    }

    if (popularView)
    {
        std::sort(filtered.begin(), filtered.end(), [](const veyra::Preset& a, const veyra::Preset& b)
                  { return popularRank(a.uuid) < popularRank(b.uuid); });
    }
    else if (recentView)
    {
        auto rank = [this](const std::string& u) {
            const auto it = std::find(recent_.begin(), recent_.end(), u);
            return it == recent_.end() ? (int) recent_.size() : (int) std::distance(recent_.begin(), it);
        };
        std::sort(filtered.begin(), filtered.end(),
                  [&](const veyra::Preset& a, const veyra::Preset& b) { return rank(a.uuid) < rank(b.uuid); });
    }
    else
    {
        std::sort(filtered.begin(), filtered.end(), [this](const veyra::Preset& a, const veyra::Preset& b)
        {
            if (sortMode_ == 1 && a.category != b.category)
                return juce::String(a.category).compareIgnoreCase(juce::String(b.category)) < 0;
            return juce::String(a.name).compareIgnoreCase(juce::String(b.name)) < 0;
        });
    }

    grid_->setFavorites(favorites_);
    grid_->setPresets(filtered, activeUuid_);
}

void PresetsScreen::selectCategory(int i)
{
    selectedCat_ = juce::jlimit(0, categories_.size() - 1, i);
    applyFilter();
    resized();
    repaint();
}

void PresetsScreen::loadFavoritesAndRecent()
{
    favorites_.clear();
    recent_.clear();
    if (auto f = veyraStateFile("favorites.txt"); f.existsAsFile())
    {
        juce::StringArray lines; lines.addLines(f.loadFileAsString());
        for (auto& l : lines) if (l.trim().isNotEmpty()) favorites_.insert(l.trim().toStdString());
    }
    if (auto f = veyraStateFile("recent.txt"); f.existsAsFile())
    {
        juce::StringArray lines; lines.addLines(f.loadFileAsString());
        for (auto& l : lines) if (l.trim().isNotEmpty()) recent_.push_back(l.trim().toStdString());
    }
}

void PresetsScreen::saveFavorites()
{
    juce::StringArray a;
    for (const auto& u : favorites_) a.add(juce::String(u));
    auto f = veyraStateFile("favorites.txt");
    f.getParentDirectory().createDirectory();
    f.replaceWithText(a.joinIntoString("\n"));
}

void PresetsScreen::saveRecent()
{
    juce::StringArray a;
    for (const auto& u : recent_) a.add(juce::String(u));
    auto f = veyraStateFile("recent.txt");
    f.getParentDirectory().createDirectory();
    f.replaceWithText(a.joinIntoString("\n"));
}

void PresetsScreen::toggleFavorite(const juce::String& uuid)
{
    const auto u = uuid.toStdString();
    if (favorites_.count(u)) favorites_.erase(u); else favorites_.insert(u);
    saveFavorites();
    // Defer the grid rebuild so we don't destroy the tile mid-click.
    juce::Component::SafePointer<PresetsScreen> safe(this);
    juce::MessageManager::callAsync([safe] { if (safe != nullptr) safe->applyFilter(); });
}

void PresetsScreen::recordRecent(const juce::String& uuid)
{
    const auto u = uuid.toStdString();
    recent_.erase(std::remove(recent_.begin(), recent_.end(), u), recent_.end());
    recent_.insert(recent_.begin(), u);
    if (recent_.size() > 12) recent_.resize(12);
    saveRecent(); // the Recently-Used view refreshes on the next setPresets
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

    // Row 1: action buttons (right-aligned).
    auto header = b.removeFromTop(36);
    int bw = 88, gap = 8;
    auto place = [&](juce::TextButton& btn)
    {
        btn.setBounds(header.removeFromRight(bw).reduced(0, 3));
        header.removeFromRight(gap);
    };
    place(deleteBtn_);
    place(dupBtn_);
    place(exportBtn_);
    place(importBtn_);
    place(saveBtn_);

    // Row 2: oval search (left) + sort + grid/list (right).
    b.removeFromTop(10);
    auto bar = b.removeFromTop(32);
    viewToggle_.setBounds(bar.removeFromRight(120).reduced(0, 2));
    bar.removeFromRight(10);
    sort_.setBounds(bar.removeFromRight(150).reduced(0, 2));
    bar.removeFromRight(14);
    search_.setBounds(bar.removeFromLeft(juce::jmin(320, bar.getWidth())));

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

    // Oval (pill) search background behind the transparent editor.
    if (search_.getWidth() > 0)
    {
        const auto sb = search_.getBounds().toFloat();
        const float radius = sb.getHeight() * 0.5f;
        g.setColour(palette_.bgGlassHover);
        g.fillRoundedRectangle(sb, radius);
        g.setColour(palette_.strokeHover);
        g.drawRoundedRectangle(sb.reduced(0.5f), radius, 1.0f);
    }

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
