#include "MiniWindow.h"

#include "Theme/Fonts.h"

namespace veyra::ui {

MiniContent::MiniContent()
{
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
}

void MiniContent::setPalette(const Palette& p)
{
    palette_ = p;
    master_.setPalette(p);
    expand_.setPalette(p);
    close_.setPalette(p);
    repaint();
}

void MiniContent::setMasterEnabled(bool on)
{
    master_.setToggleState(on, juce::dontSendNotification);
}

void MiniContent::setMasterVolume(double gain)
{
    volume_.setValue(gain, juce::dontSendNotification);
    repaint();
}

void MiniContent::setConnection(bool connected)
{
    connected_ = connected;
    repaint();
}

void MiniContent::resized()
{
    const int h = getHeight();
    auto centreY = [h](int sz) { return (h - sz) / 2; };

    int rx = getWidth() - 10;
    auto place = [&](IconButton& b, int sz) { rx -= sz; b.setBounds(rx, centreY(sz), sz, sz); rx -= 4; };
    place(close_, 28);
    place(expand_, 28);

    int x = 14 + 30 + 12;          // logo + gap
    master_.setBounds(x, centreY(20), 36, 20);
    x += 36 + 14;
    volume_.setBounds(x, centreY(16), rx - x - 12, 16);
}

void MiniContent::paint(juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat();

    // Solid glass bar (no ambient backdrop in this small window).
    g.setColour(palette_.bgCanvas);
    g.fillRoundedRectangle(b, 14.0f);
    g.setColour(palette_.strokeDefault);
    g.drawRoundedRectangle(b.reduced(0.5f), 14.0f, 1.0f);

    // Logo + connection LED.
    juce::Rectangle<float> logo(14.0f, (b.getHeight() - 28.0f) * 0.5f, 28.0f, 28.0f);
    juce::ColourGradient lg(palette_.accentPrimary, logo.getX(), logo.getY(),
                            palette_.accentPrimaryActive, logo.getRight(), logo.getBottom(), false);
    g.setGradientFill(lg);
    g.fillRoundedRectangle(logo, 8.0f);
    g.setColour(juce::Colours::white);
    g.setFont(fonts::display(14.0f));
    g.drawText("V", logo, juce::Justification::centred, false);

    const juce::Colour dot = connected_ ? juce::Colour(0xff37D67A) : palette_.warning;
    g.setColour(palette_.bgCanvas);
    g.fillEllipse(logo.getRight() - 7.0f, logo.getY() - 5.0f, 8.0f, 8.0f);
    g.setColour(dot);
    g.fillEllipse(logo.getRight() - 6.0f, logo.getY() - 4.0f, 6.0f, 6.0f);

    // Master volume percentage above the slider.
    g.setColour(palette_.textSecondary);
    g.setFont(fonts::mono(11.0f, true));
    g.drawText(juce::String(juce::roundToInt(volume_.getValue() * 100.0)) + "%",
               juce::Rectangle<int>(volume_.getRight() - 48, 4, 48, 12),
               juce::Justification::centredRight, false);
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
    : juce::DocumentWindow("Veyra Mini", juce::Colour(0xff0A0B10), 0)
{
    setUsingNativeTitleBar(false);
    setTitleBarHeight(0);
    setAlwaysOnTop(true);
    setResizable(false, false);

    auto content = std::make_unique<MiniContent>();
    content_ = content.get();
    setContentOwned(content.release(), false);
    setSize(420, 64);
    setVisible(false);
}

} // namespace veyra::ui
