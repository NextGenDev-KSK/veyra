#include "Home/TopBar.h"

#include "Theme/Fonts.h"

namespace veyra::ui {

TopBar::TopBar()
{
    logoImage_ = juce::ImageCache::getFromMemory(BinaryData::Veyra_Icon_square_png,
                                                 BinaryData::Veyra_Icon_square_pngSize);

    addAndMakeVisible(master_);
    master_.setToggleState(true, juce::dontSendNotification);
    master_.onClick = [this] { if (onMasterToggle) onMasterToggle(master_.getToggleState()); };

    volume_.setSliderStyle(juce::Slider::LinearHorizontal);
    volume_.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    volume_.setRange(0.0, 1.5, 0.01);
    volume_.setValue(1.0, juce::dontSendNotification);
    volume_.onValueChange = [this] { repaint(); if (onMasterVolume) onMasterVolume(volume_.getValue()); };
    addAndMakeVisible(volume_);

    // A/B, search and bell aren't wired yet — hide them rather than show dead
    // controls. The gear opens Settings; the window buttons work.
    for (auto* b : {&gear_, &min_, &close_})
        addAndMakeVisible(b);
    max_.setVisible(false); // fixed canvas: no maximise

    gear_.onClick  = [this] { if (onOpenSettings) onOpenSettings(); };
    min_.onClick   = [this] { if (auto* p = getPeer()) p->setMinimised(true); };
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

void TopBar::setMasterEnabled(bool on)
{
    master_.setToggleState(on, juce::dontSendNotification);
}

void TopBar::setMasterVolume(double gain)
{
    volume_.setValue(gain, juce::dontSendNotification);
    repaint();
}

void TopBar::setConnection(bool connected, juce::String version)
{
    connected_ = connected;
    version_   = std::move(version);
    repaint();
}

void TopBar::resized()
{
    const int h = getHeight();
    auto centreY = [h](int sz) { return (h - sz) / 2; };

    // Past the brand block: logo(14+36) + gap + wordmark(180) + divider, then a gap.
    int x = 256; // matches the painted divider at wordX(62)+178 plus a 16 px gap
    master_.setBounds(x, centreY(20), 36, 20);
    x += 36 + 16;
    volume_.setBounds(x, centreY(16) + 4, 200, 16); // nudged down; MASTER label painted above

    int rx = getWidth() - 16;
    auto place = [&](IconButton& b, int sz) { rx -= sz; b.setBounds(rx, centreY(sz), sz, sz); rx -= 6; };
    place(close_, 28);
    place(min_, 28); // no maximise: the window is a fixed canvas
    rx -= 10;
    place(gear_, 32); // opens Settings (A/B, search, bell hidden until wired)
}

void TopBar::paint(juce::Graphics& g)
{
    const auto h = (float) getHeight();

    // Bar fill + bottom border.
    g.setColour(palette_.bgCanvas.withAlpha(0.6f));
    g.fillRect(getLocalBounds());
    g.setColour(palette_.strokeDefault);
    g.drawLine(0.0f, h, (float) getWidth(), h, 1.0f);

    // Brand mark — the real Veyra icon, vertically centred in the bar.
    const float logoSz = 36.0f;
    juce::Rectangle<float> logo(14.0f, (h - logoSz) * 0.5f, logoSz, logoSz);
    if (logoImage_.isValid())
    {
        g.drawImage(logoImage_, logo, juce::RectanglePlacement::centred);
    }
    else // fallback if the asset failed to load
    {
        juce::ColourGradient lg(palette_.accentPrimary, logo.getX(), logo.getY(),
                                palette_.accentPrimaryActive, logo.getRight(), logo.getBottom(), false);
        g.setGradientFill(lg);
        g.fillRoundedRectangle(logo, 9.0f);
        g.setColour(juce::Colours::white);
        g.setFont(fonts::display(16.0f));
        g.drawText("V", logo, juce::Justification::centred, false);
    }

    // Connection status LED, tucked at the mark's top-right (green = connected).
    const juce::Colour dot = connected_ ? juce::Colour(0xff37D67A) : palette_.warning;
    const float dx = logo.getRight() - 4.0f, dy = logo.getY() + 4.0f;
    juce::DropShadow(dot.withAlpha(0.7f), 7, {}).drawForRectangle(
        g, juce::Rectangle<float>(dx - 5.0f, dy - 5.0f, 10.0f, 10.0f).toNearestInt());
    g.setColour(palette_.bgCanvas);
    g.fillEllipse(dx - 5.0f, dy - 5.0f, 10.0f, 10.0f);
    g.setColour(dot);
    g.fillEllipse(dx - 3.5f, dy - 3.5f, 7.0f, 7.0f);

    // Wordmark, baseline-aligned with the mark.
    const float wordX = logo.getRight() + 12.0f;
    g.setColour(palette_.textPrimary);
    g.setFont(fonts::display(17.0f));
    g.drawText("VEYRA SOUNDS", juce::Rectangle<float>(wordX, 0.0f, 180.0f, h),
               juce::Justification::centredLeft, false);

    // Divider before the master cluster.
    const float divX = wordX + 178.0f;
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

void TopBar::mouseDoubleClick(const juce::MouseEvent&) {} // fixed canvas: no maximise

} // namespace veyra::ui
