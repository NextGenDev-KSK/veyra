#include "Screens/CrashBanner.h"

#include "Theme/Fonts.h"

namespace veyra::ui {

CrashBanner::CrashBanner()
{
    open_.onClick    = [this] { if (onOpenFolder) onOpenFolder(); };
    dismiss_.onClick = [this] { if (onDismiss) onDismiss(); };
    addAndMakeVisible(open_);
    addAndMakeVisible(dismiss_);
}

void CrashBanner::setPalette(const Palette& p) { palette_ = p; repaint(); }
void CrashBanner::setMessage(juce::String text) { msg_ = std::move(text); repaint(); }

void CrashBanner::paint(juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    g.setColour(palette_.warningBg);
    g.fillRoundedRectangle(r, 10.0f);
    g.setColour(palette_.warning.withAlpha(0.6f));
    g.drawRoundedRectangle(r.reduced(0.5f), 10.0f, 1.0f);

    g.setColour(palette_.warning);
    g.fillEllipse(16.0f, r.getCentreY() - 3.0f, 6.0f, 6.0f);

    g.setColour(palette_.textPrimary);
    g.setFont(fonts::body(13.0f, true));
    g.drawText(msg_, getLocalBounds().withTrimmedLeft(34).withTrimmedRight(220),
               juce::Justification::centredLeft, true);
}

void CrashBanner::resized()
{
    auto r = getLocalBounds().reduced(10, 8);
    dismiss_.setBounds(r.removeFromRight(96));
    r.removeFromRight(8);
    open_.setBounds(r.removeFromRight(104));
}

} // namespace veyra::ui
