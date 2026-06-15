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

    // Master cluster container: a subtle accent-bordered group so the primary
    // control reads above the utility icons.
    juce::Rectangle<float> cluster((float) master_.getX() - 12.0f, (h - 40.0f) * 0.5f,
                                   (float) (volume_.getRight() + 52) - (master_.getX() - 12), 40.0f);
    g.setColour(palette_.bgGlassElevated);
    g.fillRoundedRectangle(cluster, 12.0f);
    g.setColour(palette_.strokeActive);
    g.drawRoundedRectangle(cluster.reduced(0.5f), 12.0f, 1.0f);

    // MASTER label + value around the volume slider.
    g.setColour(palette_.accentSecondary);
    g.setFont(fonts::body(10.0f, true));
    g.drawText("MASTER", juce::Rectangle<int>(volume_.getX(), 6, 200, 12),
               juce::Justification::centredLeft, false);
    g.setColour(palette_.textPrimary);
    g.setFont(fonts::mono(13.0f, true));
    g.drawText(juce::String(juce::roundToInt(volume_.getValue() * 100.0)) + "%",
               juce::Rectangle<int>(volume_.getRight() + 8, 0, 48, getHeight()),
               juce::Justification::centredLeft, false);

    // Preset selector (distinct from a search field: EQ glyph on the left, caret
    // on the right) centred between the master cluster and A/B.
    const int chipL = volume_.getRight() + 72;
    const int chipR = ab_.getX() - 16;
    if (chipR - chipL > 150)
    {
        juce::Rectangle<float> chip((float) chipL, (h - 30.0f) * 0.5f, 240.0f, 30.0f);
        g.setColour(palette_.bgGlassHover);
        g.fillRoundedRectangle(chip, 10.0f);
        g.setColour(palette_.strokeHover);
        g.drawRoundedRectangle(chip.reduced(0.5f), 10.0f, 1.0f);

        // Mini-EQ glyph (says "preset", not "search").
        const float gx = chip.getX() + 12.0f, gy = chip.getCentreY();
        const float hs[] = {6.0f, 10.0f, 7.0f};
        g.setColour(palette_.accentPrimary);
        for (int i = 0; i < 3; ++i)
            g.fillRoundedRectangle(gx + i * 4.0f, gy - hs[i] * 0.5f, 2.5f, hs[i], 1.0f);

        g.setColour(palette_.textPrimary);
        g.setFont(fonts::body(13.0f, true));
        g.drawText("FPS Footstep Boost", chip.withTrimmedLeft(28.0f).withTrimmedRight(24.0f),
                   juce::Justification::centredLeft, true);

        // Caret.
        juce::Path caret;
        const float cxr = chip.getRight() - 16.0f, cyr = chip.getCentreY();
        caret.startNewSubPath(cxr - 4.0f, cyr - 2.0f);
        caret.lineTo(cxr, cyr + 3.0f);
        caret.lineTo(cxr + 4.0f, cyr - 2.0f);
        g.setColour(palette_.textSecondary);
        g.strokePath(caret, juce::PathStrokeType(1.5f));
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
