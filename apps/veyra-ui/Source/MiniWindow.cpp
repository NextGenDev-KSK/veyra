#include "MiniWindow.h"

#include "Theme/Fonts.h"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

namespace veyra::ui {

MiniContent::MiniContent()
{
    setOpaque(false);
    logoImage_ = juce::ImageCache::getFromMemory(BinaryData::Veyra_Icon_square_png,
                                                 BinaryData::Veyra_Icon_square_pngSize);

    volume_.setSliderStyle(juce::Slider::LinearHorizontal);
    volume_.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    volume_.setRange(0.0, 1.5, 0.01);
    volume_.setValue(1.0, juce::dontSendNotification);
    volume_.onValueChange = [this] { repaint(); if (onMasterVolume) onMasterVolume(volume_.getValue()); };
    addAndMakeVisible(volume_);

    prev_.onClick = [this] { if (onPrevPreset) onPrevPreset(); };
    next_.onClick = [this] { if (onNextPreset) onNextPreset(); };
    spatial_.onClick = [this] { if (onSpatial) onSpatial(); };
    game_.onClick = [this] { if (onGame) onGame(); };
    menu_.onClick = [this] { showMenu(); };
    close_.onClick = [this] { if (onClose) onClose(); };
    for (auto* b : {&prev_, &next_, &spatial_, &game_, &menu_, &close_})
        addAndMakeVisible(*b);

    startTimerHz(30);
}

MiniContent::~MiniContent() { stopTimer(); }

void MiniContent::setPalette(const Palette& p)
{
    palette_ = p;
    for (auto* b : {&prev_, &next_, &spatial_, &game_, &menu_, &close_})
        b->setPalette(p);
    repaint();
}

void MiniContent::setMasterEnabled(bool on) { masterEnabled_ = on; repaint(); }
void MiniContent::setMasterVolume(double gain) { volume_.setValue(gain, juce::dontSendNotification); repaint(); }
void MiniContent::setConnection(bool connected) { connected_ = connected; repaint(); }
void MiniContent::setPreset(juce::String name) { preset_ = std::move(name); repaint(); }
void MiniContent::setCategory(juce::String category) { category_ = std::move(category); repaint(); }

void MiniContent::setVisualizerBars(const float* bars, int n)
{
    if (bars == nullptr || n <= 0)
        return;
    if ((int) bars_.size() != n)
        bars_.assign((size_t) n, 0.0f);
    for (int i = 0; i < n; ++i)
        bars_[(size_t) i] += (bars[i] - bars_[(size_t) i]) * 0.5f; // light smoothing
    repaint();
}

void MiniContent::setVisualizerOnly(bool on)
{
    visualizerOnly_ = on;
    for (auto* b : {&prev_, &next_, &spatial_, &game_})
        b->setVisible(! on);
    volume_.setVisible(! on);
    // menu_ + close_ stay visible so the user can leave Visualizer-Only mode.
    resized();
    repaint();
}

void MiniContent::timerCallback()
{
    // Gentle decay so the spectrum settles when the service stops feeding frames.
    for (auto& v : bars_)
        v *= 0.90f;
    repaint();
}

void MiniContent::resized()
{
    const int w = getWidth(), h = getHeight();
    auto cy = [h](int sz) { return (h - sz) / 2; };

    // Close sits in the top-right corner in both modes.
    close_.setBounds(w - 12 - 18, 8, 18, 18);

    if (visualizerOnly_)
    {
        menu_.setBounds(w - 12 - 18, h - 12 - 18, 18, 18); // bottom-right, low profile
        return;
    }

    // Right cluster (vertically centred): spatial · game · menu, right-to-left.
    int rx = w - 14;
    auto placeR = [&](IconButton& b, int sz) { rx -= sz; b.setBounds(rx, cy(sz), sz, sz); rx -= 12; };
    placeR(menu_, 22);
    placeR(game_, 32);
    placeR(spatial_, 32);
    const int rightClusterL = rx;

    // Left cluster: brand · name/category (painted) · prev · next.
    const int brandSz = 38;
    int lx = 14 + brandSz + 12 + 100 + 6; // brand + gap + name column + gap
    prev_.setBounds(lx, cy(24), 24, 24); lx += 24 + 4;
    next_.setBounds(lx, cy(24), 24, 24); lx += 24 + 14;

    // Middle: volume slider (lower half), visualizer painted above it, % to the right.
    const int pctW = 44;
    const int midR = rightClusterL - 12 - pctW;
    volume_.setBounds(lx, h / 2 + 4, juce::jmax(60, midR - lx), 14);
}

void MiniContent::drawVisualizer(juce::Graphics& g, juce::Rectangle<float> area)
{
    if (bars_.empty() || area.getWidth() < 4.0f || area.getHeight() < 2.0f)
        return;
    const int n = (int) bars_.size();
    const float slot = area.getWidth() / (float) n;
    const float barW = juce::jmax(1.5f, slot * 0.62f);
    juce::ColourGradient grad(palette_.accentSecondary, 0, area.getBottom(),
                              palette_.accentPrimary, 0, area.getY(), false);
    g.setGradientFill(grad);
    for (int i = 0; i < n; ++i)
    {
        const float v = juce::jlimit(0.0f, 1.0f, bars_[(size_t) i]);
        const float bh = juce::jmax(1.0f, v * area.getHeight());
        const float x = area.getX() + (float) i * slot + (slot - barW) * 0.5f;
        g.fillRoundedRectangle(x, area.getBottom() - bh, barW, bh, 1.0f);
    }
}

void MiniContent::paint(juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat();

    // Solid dark bar (no acrylic haze / blob) + hairline border, per the reference.
    g.setColour(palette_.bgCanvas.withAlpha(0.97f));
    g.fillRoundedRectangle(b, 16.0f);
    g.setColour(palette_.strokeDefault);
    g.drawRoundedRectangle(b.reduced(0.5f), 16.0f, 1.0f);

    if (visualizerOnly_)
    {
        drawVisualizer(g, getLocalBounds().toFloat().reduced(18.0f, 14.0f));
        return;
    }

    // Brand mark (circle + Veyra logo) with a connection LED; dimmed when muted.
    juce::Rectangle<float> ring(14.0f, (float) (getHeight() - 38) / 2.0f, 38.0f, 38.0f);
    g.setColour(palette_.bgGlass);
    g.fillEllipse(ring);
    g.setColour(masterEnabled_ ? palette_.accentPrimary.withAlpha(0.5f) : palette_.strokeDefault);
    g.drawEllipse(ring.reduced(0.5f), 1.2f);
    if (logoImage_.isValid())
    {
        juce::Graphics::ScopedSaveState s(g);
        if (! masterEnabled_)
            g.setOpacity(0.45f);
        g.drawImage(logoImage_, ring.reduced(7.0f), juce::RectanglePlacement::centred);
    }
    const juce::Colour dot = connected_ ? juce::Colour(0xff37D67A) : palette_.warning;
    g.setColour(palette_.bgCanvas);
    g.fillEllipse(ring.getRight() - 8.0f, ring.getY() - 1.0f, 9.0f, 9.0f);
    g.setColour(dot);
    g.fillEllipse(ring.getRight() - 6.5f, ring.getY() + 0.5f, 6.0f, 6.0f);

    // Preset name (bold) + category (dim), stacked in the name column.
    const float nameX = 14.0f + 38.0f + 12.0f;
    g.setColour(palette_.textPrimary);
    g.setFont(fonts::display(14.0f));
    g.drawText(preset_, juce::Rectangle<float>(nameX, 12.0f, 100.0f, 18.0f),
               juce::Justification::centredLeft, true);
    g.setColour(palette_.textTertiary);
    g.setFont(fonts::body(11.0f));
    g.drawText(category_.isNotEmpty() ? category_ : juce::String("Preset"),
               juce::Rectangle<float>(nameX, 30.0f, 100.0f, 16.0f),
               juce::Justification::centredLeft, true);

    // Visualizer sits directly above the volume slider; % to the slider's right.
    const auto vb = volume_.getBounds();
    drawVisualizer(g, juce::Rectangle<float>((float) vb.getX(), 12.0f,
                                             (float) vb.getWidth(), (float) (vb.getY() - 12.0f - 4.0f)));
    g.setColour(palette_.textSecondary);
    g.setFont(fonts::mono(11.0f, true));
    g.drawText(juce::String(juce::roundToInt(volume_.getValue() * 100.0)) + "%",
               juce::Rectangle<int>(vb.getRight() + 8, (getHeight() - 20) / 2, 44, 20),
               juce::Justification::centredLeft, false);

    // SPATIAL / GAME labels under their buttons.
    g.setColour(palette_.textTertiary);
    g.setFont(fonts::body(8.0f, true));
    auto lbl = [&](const IconButton& btn, const char* t)
    {
        g.drawText(t, juce::Rectangle<int>(btn.getX() - 6, btn.getBottom() - 2, btn.getWidth() + 12, 10),
                   juce::Justification::centred, false);
    };
    lbl(spatial_, "SPATIAL");
    lbl(game_, "GAME");
}

void MiniContent::showMenu()
{
    juce::PopupMenu m;
    m.addItem(1, "Visualizer Only", true, visualizerOnly_);
    m.addSeparator();
    m.addItem(2, "Expand");
    m.addItem(3, "Close");
    juce::Component::SafePointer<MiniContent> safe(this);
    m.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(&menu_), [safe](int r)
    {
        if (safe == nullptr) return;
        if (r == 1) safe->setVisualizerOnly(! safe->visualizerOnly_);
        else if (r == 2) { if (safe->onExpand) safe->onExpand(); }
        else if (r == 3) { if (safe->onClose) safe->onClose(); }
    });
}

void MiniContent::mouseDown(const juce::MouseEvent& e)
{
    // Clicking the brand mark toggles master; elsewhere starts a window drag.
    juce::Rectangle<int> brand(14, (getHeight() - 38) / 2, 38, 38);
    if (! visualizerOnly_ && brand.contains(e.getPosition()))
    {
        if (onMasterToggle) onMasterToggle(! masterEnabled_);
        return;
    }
    if (auto* w = getTopLevelComponent())
    {
        dragWin_ = w->getPosition();
        dragMouse_ = e.getScreenPosition();
    }
}

void MiniContent::mouseDrag(const juce::MouseEvent& e)
{
    auto* w = getTopLevelComponent();
    if (w == nullptr)
        return;
    auto pos = dragWin_ + (e.getScreenPosition() - dragMouse_);
    // Clamp to the display so the widget can never be dragged off-screen.
    if (auto* d = juce::Desktop::getInstance().getDisplays().getDisplayForPoint(e.getScreenPosition()))
    {
        const auto ua = d->userArea;
        pos.x = juce::jlimit(ua.getX(), ua.getRight() - w->getWidth(), pos.x);
        pos.y = juce::jlimit(ua.getY(), ua.getBottom() - w->getHeight(), pos.y);
    }
    w->setTopLeftPosition(pos);
}

// ---------------------------------------------------------------------------

MiniWindow::MiniWindow()
    : juce::DocumentWindow("Veyra Mini", juce::Colours::transparentBlack, 0)
{
    setUsingNativeTitleBar(false);
    setTitleBarHeight(0);
    setBackgroundColour(juce::Colours::transparentBlack);
    setAlwaysOnTop(true);
    setResizable(false, false);

    auto content = std::make_unique<MiniContent>();
    content_ = content.get();
    setContentOwned(content.release(), false);
    setSize(600, 72);
    centreWithSize(600, 72); // always open centred on the active display
    setVisible(false);
}

} // namespace veyra::ui
