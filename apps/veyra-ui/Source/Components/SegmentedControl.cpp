#include "Components/SegmentedControl.h"

#include "Theme/Fonts.h"

namespace veyra::ui {

void SegmentedControl::setSelectedIndex(int i, bool notify)
{
    i = juce::jlimit(0, juce::jmax(0, items_.size() - 1), i);
    if (i == selected_)
        return;
    selected_ = i;
    repaint();
    if (notify && onChange)
        onChange(selected_);
}

juce::Rectangle<float> SegmentedControl::segmentBounds(int index) const
{
    auto b = getLocalBounds().toFloat().reduced(4.0f);
    if (items_.isEmpty())
        return b;
    const float w = b.getWidth() / (float) items_.size();
    return {b.getX() + w * (float) index, b.getY(), w, b.getHeight()};
}

void SegmentedControl::paint(juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat();
    g.setColour(palette_.bgGlass);
    g.fillRoundedRectangle(b, b.getHeight() * 0.5f);
    g.setColour(palette_.strokeDefault);
    g.drawRoundedRectangle(b.reduced(0.5f), b.getHeight() * 0.5f, 1.0f);

    for (int i = 0; i < items_.size(); ++i)
    {
        auto seg = segmentBounds(i);
        const bool active = (i == selected_);
        if (active)
        {
            juce::DropShadow(palette_.accentGlow, 14, {}).drawForRectangle(g, seg.toNearestInt());
            g.setColour(palette_.accentPrimary);
            g.fillRoundedRectangle(seg, seg.getHeight() * 0.5f);
        }
        g.setColour(active ? palette_.textOnAccent : palette_.textSecondary);
        g.setFont(fonts::body(13.0f, true));
        g.drawText(items_[i], seg, juce::Justification::centred, false);
    }
}

void SegmentedControl::mouseDown(const juce::MouseEvent& e)
{
    for (int i = 0; i < items_.size(); ++i)
        if (segmentBounds(i).contains(e.position))
        {
            setSelectedIndex(i, true);
            return;
        }
}

} // namespace veyra::ui
