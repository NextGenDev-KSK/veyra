#include "Components/ParametricEqEditor.h"

#include "Theme/Fonts.h"

#include "eq/Biquad.h"

#include <cmath>

namespace veyra::ui {

namespace {
constexpr double kSr = 48000.0;

veyra::dsp::BiquadCoeffs coeffsFor(const veyra::ParametricBand& b)
{
    using namespace veyra::dsp;
    const double f = juce::jlimit(20.0, kSr * 0.45, (double) b.freq);
    const double q = juce::jmax(0.1f, b.q);
    switch (b.type)
    {
    case 1:  return makeLowShelf(kSr, f, q, b.gainDb);
    case 2:  return makeHighShelf(kSr, f, q, b.gainDb);
    case 3:  return makeNotch(kSr, f, q);
    case 4:  return makeHighPass(kSr, f, q);
    case 5:  return makeLowPass(kSr, f, q);
    default: return makePeaking(kSr, f, q, b.gainDb);
    }
}

float magnitudeDb(const veyra::dsp::BiquadCoeffs& c, double w)
{
    const double cw = std::cos(w), sw = std::sin(w), c2 = std::cos(2 * w), s2 = std::sin(2 * w);
    const double nRe = c.b0 + c.b1 * cw + c.b2 * c2, nIm = -(c.b1 * sw + c.b2 * s2);
    const double dRe = 1.0 + c.a1 * cw + c.a2 * c2, dIm = -(c.a1 * sw + c.a2 * s2);
    const double mag = std::sqrt((nRe * nRe + nIm * nIm) / (dRe * dRe + dIm * dIm + 1e-12));
    return (float) (20.0 * std::log10(mag + 1e-9));
}
} // namespace

ParametricEqEditor::ParametricEqEditor() { setWantsKeyboardFocus(false); }

void ParametricEqEditor::setBands(std::vector<veyra::ParametricBand> bands)
{
    bands_ = std::move(bands);
    if (selected_ >= (int) bands_.size()) selected_ = -1;
    repaint();
}

void ParametricEqEditor::setSpectrum(const float* bars, int n)
{
    if (bars == nullptr || n <= 0) { spectrum_.clear(); return; }
    spectrum_.assign(bars, bars + n);
    repaint();
}

float ParametricEqEditor::freqToX(float f) const
{
    const float t = std::log(juce::jlimit(kFMin, kFMax, f) / kFMin) / std::log(kFMax / kFMin);
    return t * (float) getWidth();
}
float ParametricEqEditor::xToFreq(float x) const
{
    const float t = juce::jlimit(0.0f, 1.0f, x / (float) getWidth());
    return kFMin * std::pow(kFMax / kFMin, t);
}
float ParametricEqEditor::gainToY(float g) const
{
    return (1.0f - (juce::jlimit(-kGMax, kGMax, g) + kGMax) / (2 * kGMax)) * (float) getHeight();
}
float ParametricEqEditor::yToGain(float y) const
{
    return juce::jlimit(-kGMax, kGMax, kGMax - (y / (float) getHeight()) * 2 * kGMax);
}

float ParametricEqEditor::curveDbAt(float freq) const
{
    double db = 0.0;
    const double w = 2.0 * juce::MathConstants<double>::pi * freq / kSr;
    for (const auto& b : bands_)
        if (b.enabled)
            db += magnitudeDb(coeffsFor(b), w);
    return (float) db;
}

int ParametricEqEditor::hitTest(juce::Point<float> p) const
{
    for (int i = 0; i < (int) bands_.size(); ++i)
    {
        const juce::Point<float> c(freqToX(bands_[(size_t) i].freq), gainToY(bands_[(size_t) i].gainDb));
        if (c.getDistanceFrom(p) <= 12.0f)
            return i;
    }
    return -1;
}

void ParametricEqEditor::paint(juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    g.setColour(palette_.bgInput.withAlpha(0.35f));
    g.fillRoundedRectangle(r, 10.0f);

    // Live FFT underlay behind the curve. The analyzer bars are already
    // log-spaced (geometric in FFT-bin index), matching this editor's
    // log-frequency x-axis, so a direct index->x mapping lines up.
    if (! spectrum_.empty())
    {
        const int n = (int) spectrum_.size();
        const float bw = r.getWidth() / (float) n;
        g.setColour(palette_.accentSecondary.withAlpha(0.16f));
        for (int i = 0; i < n; ++i)
        {
            const float bh = juce::jlimit(0.0f, 1.0f, spectrum_[(size_t) i]) * r.getHeight();
            if (bh > 0.5f)
                g.fillRect((float) i * bw, r.getHeight() - bh, bw + 1.0f, bh);
        }
    }

    // Grid: 0 dB line + a few freq guides.
    g.setColour(palette_.strokeHover.withAlpha(0.5f));
    const float midY = gainToY(0.0f);
    g.drawHorizontalLine((int) midY, 0.0f, r.getWidth());
    for (float f : {100.0f, 1000.0f, 10000.0f})
    {
        const float x = freqToX(f);
        g.drawVerticalLine((int) x, 0.0f, r.getHeight());
        g.setColour(palette_.textTertiary);
        g.setFont(fonts::mono(9.0f));
        g.drawText(f >= 1000.0f ? juce::String((int) (f / 1000)) + "k" : juce::String((int) f),
                   juce::Rectangle<float>(x + 2, r.getHeight() - 12, 30, 11),
                   juce::Justification::left, false);
        g.setColour(palette_.strokeHover.withAlpha(0.5f));
    }

    // Response curve.
    juce::Path curve;
    for (int px = 0; px <= getWidth(); px += 3)
    {
        const float y = gainToY(curveDbAt(xToFreq((float) px)));
        if (px == 0) curve.startNewSubPath((float) px, y);
        else         curve.lineTo((float) px, y);
    }
    g.setColour(palette_.accentPrimary);
    g.strokePath(curve, juce::PathStrokeType(2.0f));

    // Nodes.
    for (int i = 0; i < (int) bands_.size(); ++i)
    {
        const auto& b = bands_[(size_t) i];
        const juce::Point<float> c(freqToX(b.freq), gainToY(b.gainDb));
        const bool sel = (i == selected_);
        g.setColour(b.enabled ? palette_.accentPrimary : palette_.textTertiary);
        g.fillEllipse(c.x - 6, c.y - 6, 12, 12);
        if (sel)
        {
            g.setColour(palette_.textPrimary);
            g.drawEllipse(c.x - 8, c.y - 8, 16, 16, 1.5f);
        }
    }

    const int readout = hovered_ >= 0 ? hovered_ : selected_;
    if (readout >= 0 && readout < (int) bands_.size())
    {
        static const char* kTypes[] = {"Bell", "Low Shelf", "High Shelf", "Notch", "High Pass", "Low Pass"};
        const auto& b = bands_[(size_t) readout];
        const int t = juce::jlimit(0, 5, b.type);
        g.setColour(palette_.textSecondary);
        g.setFont(fonts::mono(10.0f));
        g.drawText(juce::String(kTypes[t]) + "   " + juce::String((int) b.freq) + " Hz   "
                       + juce::String(b.gainDb, 1) + " dB   Q " + juce::String(b.q, 2),
                   getLocalBounds().reduced(8).removeFromTop(14), juce::Justification::right, false);
    }
}

void ParametricEqEditor::mouseDown(const juce::MouseEvent& e)
{
    const int hit = hitTest(e.position);
    if (e.mods.isPopupMenu())
    {
        if (hit >= 0) showBandMenu(hit); // filter type / remove
        return;
    }
    selected_ = hit;
    dragging_ = hit;
    repaint();
}

void ParametricEqEditor::showBandMenu(int index)
{
    if (index < 0 || index >= (int) bands_.size())
        return;
    juce::PopupMenu m;
    static const char* kTypes[] = {"Bell", "Low Shelf", "High Shelf", "Notch", "High Pass", "Low Pass"};
    for (int t = 0; t < 6; ++t)
        m.addItem(t + 1, kTypes[t], true, bands_[(size_t) index].type == t);
    m.addSeparator();
    m.addItem(100, "Remove band");

    juce::Component::SafePointer<ParametricEqEditor> safe(this);
    m.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(this),
                    [safe, index](int r)
                    {
                        if (safe == nullptr || r <= 0 || index >= (int) safe->bands_.size()) return;
                        if (r == 100) { safe->bands_.erase(safe->bands_.begin() + index); safe->selected_ = -1; }
                        else          { safe->bands_[(size_t) index].type = r - 1; }
                        safe->emit();
                        safe->repaint();
                    });
}

void ParametricEqEditor::mouseMove(const juce::MouseEvent& e)
{
    const int h = hitTest(e.position);
    if (h != hovered_) { hovered_ = h; repaint(); }
    setMouseCursor(h >= 0 ? juce::MouseCursor::DraggingHandCursor : juce::MouseCursor::NormalCursor);
}

void ParametricEqEditor::mouseExit(const juce::MouseEvent&)
{
    if (hovered_ != -1) { hovered_ = -1; repaint(); }
}

void ParametricEqEditor::mouseDrag(const juce::MouseEvent& e)
{
    if (dragging_ < 0 || dragging_ >= (int) bands_.size())
        return;
    auto& b = bands_[(size_t) dragging_];
    b.freq   = juce::jlimit(kFMin, kFMax, xToFreq(e.position.x));
    b.gainDb = yToGain(e.position.y);
    emit();
    repaint();
}

void ParametricEqEditor::mouseUp(const juce::MouseEvent&) { dragging_ = -1; }

void ParametricEqEditor::mouseDoubleClick(const juce::MouseEvent& e)
{
    const int hit = hitTest(e.position);
    if (hit >= 0) // double-click a node resets its gain to flat
    {
        bands_[(size_t) hit].gainDb = 0.0f;
        emit();
        repaint();
        return;
    }
    if (bands_.size() >= 16)
        return;
    veyra::ParametricBand b;
    b.enabled = true;
    b.type = 0; // bell
    b.freq = juce::jlimit(kFMin, kFMax, xToFreq(e.position.x));
    b.gainDb = yToGain(e.position.y);
    b.q = 1.0f;
    bands_.push_back(b);
    selected_ = (int) bands_.size() - 1;
    emit();
    repaint();
}

void ParametricEqEditor::mouseWheelMove(const juce::MouseEvent&, const juce::MouseWheelDetails& w)
{
    if (selected_ < 0 || selected_ >= (int) bands_.size())
        return;
    auto& b = bands_[(size_t) selected_];
    b.q = juce::jlimit(0.1f, 10.0f, b.q * (w.deltaY > 0 ? 1.1f : 1.0f / 1.1f));
    emit();
    repaint();
}

void ParametricEqEditor::emit()
{
    if (onChanged)
        onChanged(bands_);
}

} // namespace veyra::ui
