#include "Home/Sidebar.h"

#include "Graphics/Icons.h"
#include "Theme/Fonts.h"

namespace veyra::ui {

Sidebar::Sidebar()
{
    items_ = {
        {"Home", icons::home},      {"Presets", icons::presets}, {"Apps", icons::apps},
        {"Devices", icons::devices}, {"Sound Lab", icons::flask}, {"Gamer Mode", icons::gamer},
        {"Settings", icons::gear},
    };
}

juce::Rectangle<int> Sidebar::itemBounds(int i) const
{
    const int w = getWidth();
    if (i < 6)
        return {8, 24 + i * 44, w - 16, 40};
    // Settings sits just above the divider / mini button.
    return {8, miniBounds().getY() - 16 - 40, w - 16, 40};
}

juce::Rectangle<int> Sidebar::miniBounds() const
{
    const int w = getWidth();
    return {12, getHeight() - 24 - 8 - 32, w - 24, 32};
}

void Sidebar::paint(juce::Graphics& g)
{
    g.setColour(palette_.bgApp.withAlpha(0.4f));
    g.fillRect(getLocalBounds());
    g.setColour(palette_.strokeDefault);
    g.drawLine((float) getWidth(), 0.0f, (float) getWidth(), (float) getHeight(), 1.0f);

    for (int i = 0; i < (int) items_.size(); ++i)
    {
        auto r = itemBounds(i);
        const bool active = (i == active_);
        const bool hover = (i == hover_);

        if (active)
        {
            g.setColour(palette_.bgGlassActive);
            g.fillRoundedRectangle(r.toFloat(), 10.0f);
            g.setColour(palette_.accentPrimary);
            g.fillRoundedRectangle(juce::Rectangle<float>((float) r.getX(), (float) r.getY() + 8.0f,
                                                          3.0f, (float) r.getHeight() - 16.0f), 1.5f);
        }
        else if (hover)
        {
            g.setColour(palette_.bgGlassHover);
            g.fillRoundedRectangle(r.toFloat(), 10.0f);
        }

        const juce::Colour col = active ? palette_.accentPrimary
                                 : hover ? palette_.textPrimary : palette_.textSecondary;
        auto iconBox = juce::Rectangle<float>((float) r.getX() + 12.0f, (float) r.getCentreY() - 9.0f, 18.0f, 18.0f);
        items_[(size_t) i].icon(g, iconBox, col);

        g.setColour(active || hover ? palette_.textPrimary : palette_.textSecondary);
        g.setFont(fonts::body(14.0f, false));
        g.drawText(items_[(size_t) i].label, r.withTrimmedLeft(42), juce::Justification::centredLeft, false);
    }

    // Divider above Settings.
    const int divY = itemBounds(6).getY() - 8;
    g.setColour(palette_.strokeDefault);
    g.drawLine(20.0f, (float) divY, (float) getWidth() - 20.0f, (float) divY, 1.0f);

    // Mini Mode button.
    auto mini = miniBounds().toFloat();
    g.setColour(palette_.strokeDefault);
    g.drawRoundedRectangle(mini.reduced(0.5f), 10.0f, 1.0f);
    g.setColour(palette_.textSecondary);
    g.setFont(fonts::mono(11.0f, true));
    g.drawText("MINI MODE", mini, juce::Justification::centred, false);

    // Version.
    g.setColour(palette_.textTertiary);
    g.setFont(fonts::mono(11.0f));
    g.drawText("v0.3.0", juce::Rectangle<int>(0, getHeight() - 22, getWidth(), 16),
               juce::Justification::centred, false);
}

void Sidebar::mouseDown(const juce::MouseEvent& e)
{
    for (int i = 0; i < (int) items_.size(); ++i)
        if (itemBounds(i).contains(e.getPosition()))
        {
            active_ = i;
            repaint();
            if (onNavigate)
                onNavigate(i);
            return;
        }
}

void Sidebar::mouseMove(const juce::MouseEvent& e)
{
    int h = -1;
    for (int i = 0; i < (int) items_.size(); ++i)
        if (itemBounds(i).contains(e.getPosition()))
            h = i;
    if (h != hover_) { hover_ = h; repaint(); }
}

void Sidebar::mouseExit(const juce::MouseEvent&)
{
    if (hover_ != -1) { hover_ = -1; repaint(); }
}

} // namespace veyra::ui
