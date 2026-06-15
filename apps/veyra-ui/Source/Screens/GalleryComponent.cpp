#include "Screens/GalleryComponent.h"

#include "Theme/Fonts.h"

namespace veyra::ui {

GalleryComponent::GalleryComponent()
{
    buttonsCard_.setTitle("Buttons");
    controlsCard_.setTitle("Controls");
    themesCard_.setTitle("Themes");
    addAndMakeVisible(buttonsCard_);
    addAndMakeVisible(controlsCard_);
    addAndMakeVisible(themesCard_);

    primaryButton_.getProperties().set("variant", "primary");
    ghostButton_.getProperties().set("variant", "ghost");
    dangerButton_.getProperties().set("variant", "danger");
    addAndMakeVisible(primaryButton_);
    addAndMakeVisible(ghostButton_);
    addAndMakeVisible(dangerButton_);

    toggle_.setToggleState(true, juce::dontSendNotification);
    addAndMakeVisible(toggle_);

    hslider_.setSliderStyle(juce::Slider::LinearHorizontal);
    hslider_.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    hslider_.setRange(0.0, 1.0);
    hslider_.setValue(0.6);
    addAndMakeVisible(hslider_);

    knob_.setLabel("Bass Boost");
    knob_.setValue(0.33);
    knob_.setValueText("+4 dB");
    knob_.onChange = [this](double v)
    {
        knob_.setValueText(juce::String(juce::roundToInt(v * 12.0)) + " dB");
    };
    addAndMakeVisible(knob_);

    segmented_.setItems({"Graphic", "Parametric"});
    addAndMakeVisible(segmented_);

    for (const auto& theme : builtInThemes())
    {
        auto* b = new juce::TextButton(theme.name);
        b->getProperties().set("variant", "ghost");
        const juce::String id = theme.id;
        b->onClick = [this, id] { if (onThemeSelected) onThemeSelected(id); };
        addAndMakeVisible(b);
        themeButtons_.add(b);
    }
}

void GalleryComponent::setPalette(const Palette& p)
{
    palette_ = p;
    for (auto* c : {&buttonsCard_, &controlsCard_, &themesCard_})
        c->setPalette(p);
    toggle_.setPalette(p);
    knob_.setPalette(p);
    segmented_.setPalette(p);
    repaint();
}

void GalleryComponent::paint(juce::Graphics& g)
{
    auto header = getLocalBounds().reduced(24).removeFromTop(40);
    g.setColour(palette_.textPrimary);
    g.setFont(fonts::display(28.0f));
    g.drawText("FOUNDATIONS", header, juce::Justification::centredLeft, false);
}

void GalleryComponent::resized()
{
    auto area = getLocalBounds().reduced(24);
    area.removeFromTop(40 + 16); // header + gap

    auto buttonsBounds = area.removeFromTop(150);
    buttonsCard_.setBounds(buttonsBounds);
    area.removeFromTop(24);

    auto controlsBounds = area.removeFromTop(220);
    controlsCard_.setBounds(controlsBounds);
    area.removeFromTop(24);

    themesCard_.setBounds(area);

    // ---- Buttons card content ----
    auto bc = buttonsCard_.contentArea() + buttonsCard_.getPosition();
    auto row = bc.removeFromTop(36);
    primaryButton_.setBounds(row.removeFromLeft(120));
    row.removeFromLeft(12);
    ghostButton_.setBounds(row.removeFromLeft(120));
    row.removeFromLeft(12);
    dangerButton_.setBounds(row.removeFromLeft(120));
    bc.removeFromTop(20);
    toggle_.setBounds(bc.removeFromTop(24).removeFromLeft(36));

    // ---- Controls card content ----
    auto cc = controlsCard_.contentArea() + controlsCard_.getPosition();
    hslider_.setBounds(cc.removeFromTop(40).reduced(0, 8));
    cc.removeFromTop(16);
    auto lower = cc.removeFromTop(110);
    knob_.setBounds(lower.removeFromLeft(120));
    lower.removeFromLeft(24);
    segmented_.setBounds(lower.removeFromTop(36).removeFromLeft(240));

    // ---- Themes card content (wrapping grid) ----
    auto tc = themesCard_.contentArea() + themesCard_.getPosition();
    const int bw = 124, bh = 36, gap = 12;
    int x = tc.getX(), y = tc.getY();
    for (auto* b : themeButtons_)
    {
        if (x + bw > tc.getRight())
        {
            x = tc.getX();
            y += bh + gap;
        }
        b->setBounds(x, y, bw, bh);
        x += bw + gap;
    }
}

} // namespace veyra::ui
