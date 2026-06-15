#include "Home/TopBar.h"

#include "Theme/Fonts.h"

namespace veyra::ui {

TopBar::TopBar()
{
    addAndMakeVisible(master_);
    master_.setToggleState(true, juce::dontSendNotification);

    volume_.setSliderStyle(juce::Slider::LinearHorizontal);
    volume_.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    volume_.setRange(0.0, 1.5, 0.01);
    volume_.setValue(1.0, juce::dontSendNotification);
    volume_.onValueChange = [this] { repaint(); };
    addAndMakeVisible(volume_);

    ab_.getProperties().set("variant", "ghost");
    addAndMakeVisible(ab_);

    for (auto* b : {&search_, &bell_, &gear_, &min_, &max_, &close_})
        addAndMakeVisible(b);

    min_.onClick = [this] { if (auto* p = getPeer()) p->setMinimised(true); };
    max_.onClick = [this] { toggleMaximise(); };
    close_.onClick = [] { juce::JUCEApplication::getInstance()->systemRequestedQuit(); };
}

void TopBar::setPalette(const Palette& p)
{
    palette_ = p;
    master_.setPalette(p);
    for (auto* b : {&search_, &bell_, &gear_, &min_, &max_, &close_})
        b->setPalette(p);
    repaint();
}

void TopBar::toggleMaximise()
{
    if (auto* w = dynamic_cast<juce::ResizableWindow*>(getTopLevelComponent()))
        w->setFullScreen(!w->isFullScreen());
}

void TopBar::resized()
{
    const int h = getHeight();
    auto centreY = [h](int sz) { return (h - sz) / 2; };

    int x = 16 + 28 + 8 + 150 + 16 + 1 + 16; // logo + wordmark + divider (painted)
    master_.setBounds(x, centreY(20), 36, 20);
    x += 36 + 16;
    volume_.setBounds(x, centreY(16) + 4, 200, 16); // nudged down; MASTER label painted above

    int rx = getWidth() - 16;
    auto place = [&](IconButton& b, int sz) { rx -= sz; b.setBounds(rx, centreY(sz), sz, sz); rx -= 6; };
    place(close_, 28);
    place(max_, 28);
    place(min_, 28);
    rx -= 10;
    place(gear_, 32);
    place(bell_, 32);
    place(search_, 32);
    rx -= 6;
    rx -= 44; ab_.setBounds(rx, centreY(28), 44, 28);
}

void TopBar::paint(juce::Graphics& g)
{
    const auto h = (float) getHeight();

    // Bar fill + bottom border.
    g.setColour(palette_.bgCanvas.withAlpha(0.6f));
    g.fillRect(getLocalBounds());
    g.setColour(palette_.strokeDefault);
    g.drawLine(0.0f, h, (float) getWidth(), h, 1.0f);

    // Logo.
    juce::Rectangle<float> logo(16.0f, (h - 28.0f) * 0.5f, 28.0f, 28.0f);
    juce::ColourGradient lg(palette_.accentPrimary, logo.getX(), logo.getY(),
                            palette_.accentPrimaryActive, logo.getRight(), logo.getBottom(), false);
    g.setGradientFill(lg);
    g.fillRoundedRectangle(logo, 8.0f);
    g.setColour(juce::Colours::white);
    g.setFont(fonts::display(14.0f));
    g.drawText("V", logo, juce::Justification::centred, false);

    // Wordmark.
    g.setColour(palette_.textPrimary);
    g.setFont(fonts::display(15.0f));
    g.drawText("VEYRA SOUNDS", juce::Rectangle<int>(52, 0, 150, getHeight()),
               juce::Justification::centredLeft, false);

    // Divider before master toggle.
    const float divX = (float) (52 + 150 + 8);
    g.setColour(palette_.strokeDefault);
    g.drawLine(divX, h * 0.25f, divX, h * 0.75f, 1.0f);

    // MASTER label + value around the volume slider.
    g.setColour(palette_.textTertiary);
    g.setFont(fonts::body(10.0f, true));
    g.drawText("MASTER", juce::Rectangle<int>(volume_.getX(), 6, 200, 12),
               juce::Justification::centredLeft, false);
    g.setColour(palette_.textSecondary);
    g.setFont(fonts::mono(12.0f));
    g.drawText(juce::String(juce::roundToInt(volume_.getValue() * 100.0)) + "%",
               juce::Rectangle<int>(volume_.getRight() + 8, 0, 48, getHeight()),
               juce::Justification::centredLeft, false);

    // Preset chip (decorative for now) centred between the volume area and A/B.
    const int chipL = volume_.getRight() + 64;
    const int chipR = ab_.getX() - 16;
    if (chipR - chipL > 120)
    {
        juce::Rectangle<float> chip((float) chipL, (h - 28.0f) * 0.5f, 220.0f, 28.0f);
        g.setColour(palette_.bgGlassHover);
        g.fillRoundedRectangle(chip, 14.0f);
        g.setColour(palette_.strokeDefault);
        g.drawRoundedRectangle(chip.reduced(0.5f), 14.0f, 1.0f);
        g.setColour(palette_.accentPrimary);
        g.fillEllipse(chip.getX() + 12.0f, chip.getCentreY() - 3.0f, 6.0f, 6.0f);
        g.setColour(palette_.textPrimary);
        g.setFont(fonts::body(12.0f, true));
        g.drawText("FPS Footstep Boost", chip.withTrimmedLeft(28.0f),
                   juce::Justification::centredLeft, true);
    }
}

void TopBar::mouseDown(const juce::MouseEvent& e)
{
    if (auto* w = getTopLevelComponent())
    {
        dragWin_ = w->getPosition();
        dragMouse_ = e.getScreenPosition();
    }
}

void TopBar::mouseDrag(const juce::MouseEvent& e)
{
    if (auto* w = getTopLevelComponent())
        w->setTopLeftPosition(dragWin_ + (e.getScreenPosition() - dragMouse_));
}

void TopBar::mouseDoubleClick(const juce::MouseEvent&) { toggleMaximise(); }

} // namespace veyra::ui
