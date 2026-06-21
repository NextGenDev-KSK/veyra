#pragma once

// Presets screen — Phase 5. A scrollable grid of preset tiles (built-in + user)
// with the active one highlighted. Header actions: save current settings as a
// new preset, import a .vpreset, and export/delete the selected one. Content
// only; the shell wires the callbacks to the service client.

#include "Components/SegmentedControl.h"
#include "Theme/DesignTokens.h"
#include "VeyraGui.h"

#include "veyra/Preset.h"

#include <functional>
#include <memory>
#include <vector>

namespace veyra::ui {

class GlassBackground;

class PresetsScreen : public juce::Component {
public:
    PresetsScreen();
    ~PresetsScreen() override;

    void setPalette(const Palette& p);
    void attachBackdrop(GlassBackground* bg);
    void setPresets(std::vector<veyra::Preset> presets, juce::String activeUuid);

    void resized() override;
    void paint(juce::Graphics&) override;
    void mouseDown(const juce::MouseEvent& e) override; // category column selection

    std::function<void(juce::String)> onApply;       // preset uuid
    std::function<void(juce::String)> onDelete;      // preset uuid
    std::function<void(juce::String)> onExport;      // preset uuid
    std::function<void(juce::String)> onDuplicate;   // preset uuid -> clone as user preset
    std::function<void(juce::String)> onSaveCurrent; // new preset name
    std::function<void()>             onImport;

private:
    class Tile;
    class Grid;
    void promptSaveCurrent();
    void applyFilter();
    void selectCategory(int i);
    juce::Rectangle<int> categoryColumn() const;
    juce::Rectangle<int> categoryItemBounds(int i) const;

    Palette          palette_  = paletteForTheme("midnight");
    GlassBackground* backdrop_ = nullptr;
    juce::String     activeUuid_;

    std::vector<veyra::Preset> allPresets_;
    juce::StringArray          categories_{"All Presets"};
    int                        selectedCat_ = 0;

    juce::TextEditor       search_;
    SegmentedControl       viewToggle_;
    SegmentedControl       sort_;
    int                    sortMode_ = 0; // 0 = A-Z, 1 = Category
    juce::Viewport         viewport_;
    std::unique_ptr<Grid>  grid_;
    juce::TextButton saveBtn_{"Save Current"}, importBtn_{"Import"},
                     exportBtn_{"Export"}, dupBtn_{"Duplicate"}, deleteBtn_{"Delete"};
};

} // namespace veyra::ui
