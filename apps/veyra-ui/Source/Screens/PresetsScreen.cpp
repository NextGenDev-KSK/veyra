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

    int columnsFor(int width) const { return juce::jmax(1, (width + kGap) / (kTileW + kGap)); }

    int preferredHeight(int width) const
    {
        if (tiles_.empty())
            return 0;
        const int cols = columnsFor(width);
        const int rows = ((int) tiles_.size() + cols - 1) / cols;
        return rows * kTileH + (rows - 1) * kGap;
    }

    void resized() override
    {
        const int cols = columnsFor(getWidth());
        const int tileW = (getWidth() - kGap * (cols - 1)) / juce::jmax(1, cols);
        for (int i = 0; i < (int) tiles_.size(); ++i)
        {
            const int col = i % cols, row = i / cols;
            tiles_[(size_t) i]->setBounds(col * (tileW + kGap), row * (kTileH + kGap), tileW, kTileH);
        }
    }

private:
    std::vector<std::unique_ptr<Tile>> tiles_;
    GlassBackground* backdrop_ = nullptr;
    Palette palette_ = paletteForTheme("midnight");
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
}

PresetsScreen::~PresetsScreen() = default;

void PresetsScreen::setPalette(const Palette& p)
{
    palette_ = p;
    grid_->setPalette(p);
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
    grid_->setPresets(presets, activeUuid);
    resized();
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

    auto header = b.removeFromTop(40);
    // Action buttons on the right.
    int bw = 96, gap = 8;
    auto place = [&](juce::TextButton& btn)
    {
        btn.setBounds(header.removeFromRight(bw).reduced(0, 4));
        header.removeFromRight(gap);
    };
    place(deleteBtn_);
    place(exportBtn_);
    place(importBtn_);
    place(saveBtn_);

    b.removeFromTop(12);
    viewport_.setBounds(b);

    const int w = viewport_.getMaximumVisibleWidth();
    grid_->setBounds(0, 0, w, grid_->preferredHeight(w));
}

void PresetsScreen::paint(juce::Graphics& g)
{
    auto header = getLocalBounds().reduced(kPad).removeFromTop(40);
    g.setColour(palette_.textPrimary);
    g.setFont(fonts::display(24.0f));
    g.drawText("PRESETS", header, juce::Justification::centredLeft, false);
}

} // namespace veyra::ui
