#include "Theme/VeyraLookAndFeel.h"

namespace veyra::ui {

namespace {
constexpr float kButtonRadius = 10.0f;
constexpr float kComboRadius = 10.0f;
juce::Colour trackColour() { return juce::Colour(60, 62, 80).withAlpha(0.6f); }

juce::String variantOf(const juce::Button& b)
{
    return b.getProperties().getWithDefault("variant", "ghost").toString();
}
} // namespace

VeyraLookAndFeel::VeyraLookAndFeel()
{
    fonts::initialise();
    setPalette(palette_);
}

void VeyraLookAndFeel::setPalette(const Palette& p)
{
    palette_ = p;

    setColour(juce::PopupMenu::backgroundColourId, p.bgGlassElevated);
    setColour(juce::PopupMenu::textColourId, p.textPrimary);
    setColour(juce::PopupMenu::highlightedBackgroundColourId, p.bgGlassHover);
    setColour(juce::PopupMenu::highlightedTextColourId, p.textPrimary);

    setColour(juce::ComboBox::backgroundColourId, p.bgGlass);
    setColour(juce::ComboBox::textColourId, p.textPrimary);
    setColour(juce::ComboBox::arrowColourId, p.textSecondary);
    setColour(juce::ComboBox::outlineColourId, p.strokeDefault);

    setColour(juce::Label::textColourId, p.textPrimary);
    setColour(juce::TextButton::textColourOnId, p.textPrimary);
    setColour(juce::TextButton::textColourOffId, p.textPrimary);
    setColour(juce::Slider::textBoxTextColourId, p.textPrimary);
    setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
}

juce::Font VeyraLookAndFeel::getTextButtonFont(juce::TextButton&, int)
{
    return fonts::body(13.0f, true);
}

void VeyraLookAndFeel::drawButtonBackground(juce::Graphics& g, juce::Button& button,
                                            const juce::Colour&,
                                            bool over, bool down)
{
    const auto b = button.getLocalBounds().toFloat();
    const auto variant = variantOf(button);
    const bool enabled = button.isEnabled();

    if (variant == "primary")
    {
        juce::Colour fill = !enabled ? palette_.accentPrimary.withAlpha(0.30f)
                            : down   ? palette_.accentPrimaryActive
                            : over   ? palette_.accentPrimaryHover
                                     : palette_.accentPrimary;
        if (enabled)
        {
            juce::DropShadow(palette_.accentGlow, over ? 18 : 12, {})
                .drawForRectangle(g, button.getLocalBounds());
        }
        g.setColour(fill);
        g.fillRoundedRectangle(b, kButtonRadius);
    }
    else // ghost / danger share the translucent fill + 1px border
    {
        juce::Colour fill = down ? palette_.bgGlassActive
                            : over ? palette_.bgGlassHover
                                   : juce::Colours::transparentBlack;
        g.setColour(fill);
        g.fillRoundedRectangle(b, kButtonRadius);

        juce::Colour border = variant == "danger"
                                  ? palette_.danger.withAlpha(0.40f)
                                  : (over ? palette_.strokeHover : palette_.strokeDefault);
        g.setColour(border);
        g.drawRoundedRectangle(b.reduced(0.5f), kButtonRadius, 1.0f);
    }
}

void VeyraLookAndFeel::drawButtonText(juce::Graphics& g, juce::TextButton& button,
                                      bool, bool)
{
    const auto variant = variantOf(button);
    juce::Colour col = variant == "primary" ? palette_.textOnAccent
                       : variant == "danger" ? palette_.danger
                                             : palette_.textPrimary;
    if (!button.isEnabled())
        col = palette_.textDisabled;

    g.setColour(col);
    g.setFont(getTextButtonFont(button, button.getHeight()));
    g.drawText(button.getButtonText(), button.getLocalBounds(),
               juce::Justification::centred, true);
}

void VeyraLookAndFeel::drawLinearSlider(juce::Graphics& g, int x, int y, int width,
                                        int height, float sliderPos, float, float,
                                        juce::Slider::SliderStyle style,
                                        juce::Slider& slider)
{
    if (style != juce::Slider::LinearHorizontal)
    {
        LookAndFeel_V4::drawLinearSlider(g, x, y, width, height, sliderPos,
                                         0.0f, 0.0f, style, slider);
        return;
    }

    const float trackH = 4.0f;
    const float cy = (float) y + (float) height * 0.5f;
    juce::Rectangle<float> track((float) x, cy - trackH * 0.5f, (float) width, trackH);

    g.setColour(trackColour());
    g.fillRoundedRectangle(track, 2.0f);

    g.setColour(palette_.accentPrimary);
    g.fillRoundedRectangle({track.getX(), track.getY(), sliderPos - (float) x, trackH}, 2.0f);

    const float r = 9.0f;
    g.setColour(juce::Colours::black.withAlpha(0.40f));
    g.fillEllipse(sliderPos - r, cy - r + 2.0f, r * 2.0f, r * 2.0f);
    g.setColour(juce::Colours::white);
    g.fillEllipse(sliderPos - r, cy - r, r * 2.0f, r * 2.0f);
    g.setColour(palette_.accentPrimary);
    g.drawEllipse(sliderPos - r, cy - r, r * 2.0f, r * 2.0f, 2.0f);
}

juce::Font VeyraLookAndFeel::getComboBoxFont(juce::ComboBox&)
{
    return fonts::body(14.0f);
}

void VeyraLookAndFeel::drawComboBox(juce::Graphics& g, int width, int height, bool,
                                    int, int, int, int, juce::ComboBox&)
{
    auto b = juce::Rectangle<float>(0.0f, 0.0f, (float) width, (float) height);
    g.setColour(palette_.bgGlass);
    g.fillRoundedRectangle(b, kComboRadius);
    g.setColour(palette_.strokeDefault);
    g.drawRoundedRectangle(b.reduced(0.5f), kComboRadius, 1.0f);

    juce::Path chevron;
    const float cx = (float) width - 16.0f;
    const float cyc = (float) height * 0.5f;
    chevron.startNewSubPath(cx - 4.0f, cyc - 2.0f);
    chevron.lineTo(cx, cyc + 3.0f);
    chevron.lineTo(cx + 4.0f, cyc - 2.0f);
    g.setColour(palette_.textSecondary);
    g.strokePath(chevron, juce::PathStrokeType(1.5f));
}

void VeyraLookAndFeel::drawPopupMenuBackground(juce::Graphics& g, int width, int height)
{
    g.fillAll(palette_.bgGlassElevated);
    g.setColour(palette_.strokeHover);
    g.drawRect(0, 0, width, height, 1);
}

} // namespace veyra::ui
