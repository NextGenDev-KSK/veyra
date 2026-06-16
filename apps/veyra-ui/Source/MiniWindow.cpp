#include "MiniWindow.h"

#include "Theme/Fonts.h"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <dwmapi.h>

namespace veyra::ui {

namespace {
void enableAcrylicBackdrop(HWND hwnd)
{
    BOOL dark = TRUE;
    DwmSetWindowAttribute(hwnd, 20 /*DWMWA_USE_IMMERSIVE_DARK_MODE*/, &dark, sizeof(dark));
    int backdrop = 3; // DWMSBT_TRANSIENTWINDOW (acrylic)
    DwmSetWindowAttribute(hwnd, 38 /*DWMWA_SYSTEMBACKDROP_TYPE*/, &backdrop, sizeof(backdrop));
}
} // namespace

MiniContent::MiniContent()
{
    setOpaque(false);
    logoImage_ = juce::ImageCache::getFromMemory(BinaryData::Veyra_Icon_square_png,
                                                 BinaryData::Veyra_Icon_square_pngSize);

    master_.setToggleState(true, juce::dontSendNotification);
    master_.onClick = [this] { if (onMasterToggle) onMasterToggle(master_.getToggleState()); };
    addAndMakeVisible(master_);

    volume_.setSliderStyle(juce::Slider::LinearHorizontal);
    volume_.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    volume_.setRange(0.0, 1.5, 0.01);
    volume_.setValue(1.0, juce::dontSendNotification);
    volume_.onValueChange = [this] { repaint(); if (onMasterVolume) onMasterVolume(volume_.getValue()); };
    addAndMakeVisible(volume_);

    expand_.onClick = [this] { if (onExpand) onExpand(); };
    close_.onClick  = [this] { if (onClose) onClose(); };
    addAndMakeVisible(expand_);
    addAndMakeVisible(close_);

    startTimerHz(30);
}

MiniContent::~MiniContent() { stopTimer(); }

void MiniContent::setPalette(const Palette& p)
{
    palette_ = p;
    master_.setPalette(p);
    expand_.setPalette(p);
    close_.setPalette(p);
    repaint();
}

void MiniContent::setMasterEnabled(bool on) { master_.setToggleState(on, juce::dontSendNotification); }
void MiniContent::setMasterVolume(double gain) { volume_.setValue(gain, juce::dontSendNotification); repaint(); }
void MiniContent::setConnection(bool connected) { connected_ = connected; repaint(); }
void MiniContent::setPreset(juce::String name) { preset_ = std::move(name); repaint(); }

void MiniContent::timerCallback()
{
    if (++frame_ % 6 == 0)
        for (auto& t : peakTarget_)
            t = 0.15f + 0.7f * rng_.nextFloat();
    for (size_t i = 0; i < peak_.size(); ++i)
        peak_[i] += (peakTarget_[i] - peak_[i]) * 0.2f;
    repaint();
}

void MiniContent::resized()
{
    const int h = getHeight();
    auto centreY = [h](int sz) { return (h - sz) / 2; };

    // Right: close, expand.
    int rx = getWidth() - 12;
    auto place = [&](IconButton& b, int sz) { rx -= sz; b.setBounds(rx, centreY(sz), sz, sz); rx -= 4; };
    place(close_, 26);
    place(expand_, 26);
    rx -= 8;
    rx -= 24; // peak meters column (painted)

    // Bottom row: mute + volume under the preset label.
    const int left = 12 + 32 + 12; // logo + gap
    master_.setBounds(left, h - 30, 36, 20);
    const int volX = left + 44;
    volume_.setBounds(volX, h - 28, rx - volX - 44, 16);
}

void MiniContent::paint(juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat();

    // Translucent acrylic base + hairline border.
    g.setColour(palette_.bgCanvas.withAlpha(0.55f));
    g.fillRoundedRectangle(b, 16.0f);
    g.setColour(palette_.strokeDefault);
    g.drawRoundedRectangle(b.reduced(0.5f), 16.0f, 1.0f);

    // Brand mark + connection LED.
    juce::Rectangle<float> logo(12.0f, 12.0f, 32.0f, 32.0f);
    if (logoImage_.isValid())
        g.drawImage(logoImage_, logo, juce::RectanglePlacement::centred);
    const juce::Colour dot = connected_ ? juce::Colour(0xff37D67A) : palette_.warning;
    g.setColour(palette_.bgCanvas);
    g.fillEllipse(logo.getRight() - 8.0f, logo.getY() - 3.0f, 9.0f, 9.0f);
    g.setColour(dot);
    g.fillEllipse(logo.getRight() - 6.5f, logo.getY() - 1.5f, 6.0f, 6.0f);

    const float left = 12.0f + 32.0f + 12.0f;

    // Current preset (top) + master % (right of it).
    g.setColour(palette_.textPrimary);
    g.setFont(fonts::display(13.0f));
    g.drawText(preset_, juce::Rectangle<float>(left, 9.0f, (float) getWidth() - left - 150.0f, 18.0f),
               juce::Justification::centredLeft, true);

    g.setColour(palette_.textSecondary);
    g.setFont(fonts::mono(11.0f, true));
    g.drawText(juce::String(juce::roundToInt(volume_.getValue() * 100.0)) + "%",
               juce::Rectangle<int>(volume_.getRight() + 6, getHeight() - 30, 40, 20),
               juce::Justification::centredLeft, false);

    // Slim peak meters between the volume readout and the buttons.
    const float mx = (float) (getWidth() - 12 - 26 - 4 - 26 - 8 - 24);
    const float my = 14.0f, mh = (float) getHeight() - 28.0f;
    for (int i = 0; i < 2; ++i)
    {
        const float x = mx + i * 9.0f;
        g.setColour(juce::Colour(60, 62, 80).withAlpha(0.5f));
        g.fillRoundedRectangle(x, my, 5.0f, mh, 2.5f);
        const float fh = mh * peak_[(size_t) i];
        juce::ColourGradient grad(palette_.accentSecondary, 0, my + mh, palette_.accentPrimary, 0, my, false);
        g.setGradientFill(grad);
        g.fillRoundedRectangle(x, my + mh - fh, 5.0f, fh, 2.5f);
    }
}

void MiniContent::mouseDown(const juce::MouseEvent& e)
{
    if (auto* w = getTopLevelComponent())
    {
        dragWin_ = w->getPosition();
        dragMouse_ = e.getScreenPosition();
    }
}

void MiniContent::mouseDrag(const juce::MouseEvent& e)
{
    if (auto* w = getTopLevelComponent())
        w->setTopLeftPosition(dragWin_ + (e.getScreenPosition() - dragMouse_));
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
    setSize(420, 72);
    setVisible(false);

    if (auto* peer = getPeer())
        enableAcrylicBackdrop(static_cast<HWND>(peer->getNativeHandle()));
}

} // namespace veyra::ui
