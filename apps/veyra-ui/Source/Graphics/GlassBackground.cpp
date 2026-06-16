#include "Graphics/GlassBackground.h"

namespace veyra::ui {

void GlassBackground::renderSharp(juce::Image& img) const
{
    juce::Graphics g(img);
    const auto b = juce::Rectangle<float>(0.0f, 0.0f, (float) img.getWidth(), (float) img.getHeight());

    g.fillAll(palette_.bgApp);

    // Solid mode: just the flat base — no ambient blobs.
    if (bgMode_ == 1)
        return;

    // Image mode (2) has no user image yet: a subtle diagonal accent wash stands
    // in. Ambient mode (0) draws the full set of accent blobs.
    if (bgMode_ == 2)
    {
        juce::ColourGradient wash(palette_.accentPrimary.withAlpha(0.18f * opacity_),
                                  0.0f, 0.0f,
                                  palette_.bgApp.withAlpha(0.0f), b.getWidth(), b.getHeight(), false);
        g.setGradientFill(wash);
        g.fillRect(b);
        return;
    }

    struct Blob { float rx, ry, rr, a; juce::Colour c; };
    const Blob blobs[] = {
        {0.15f, 0.12f, 0.55f, 0.32f, palette_.accentPrimary},
        {0.90f, 0.22f, 0.45f, 0.14f, palette_.accentSecondary},
        {0.72f, 0.98f, 0.62f, 0.22f, palette_.accentPrimary},
        {0.02f, 0.99f, 0.42f, 0.12f, palette_.info},
    };
    for (const auto& blob : blobs)
    {
        const float cx = b.getWidth() * blob.rx;
        const float cy = b.getHeight() * blob.ry;
        const float rad = juce::jmax(b.getWidth(), b.getHeight()) * blob.rr;
        const juce::Colour c = blob.c.withAlpha(blob.a * opacity_); // opacity scales intensity
        juce::ColourGradient grad(c, cx, cy, c.withAlpha(0.0f), cx + rad, cy, true);
        g.setGradientFill(grad);
        g.fillEllipse(cx - rad, cy - rad, rad * 2.0f, rad * 2.0f);
    }
}

void GlassBackground::rebuild()
{
    const int w = juce::jmax(1, getWidth());
    const int h = juce::jmax(1, getHeight());

    sharp_ = juce::Image(juce::Image::ARGB, w, h, true);
    renderSharp(sharp_);

    // Downscale, gaussian-blur the small copy; upscaling at draw time smooths further.
    const int lw = juce::jmax(1, w / kDownscale);
    const int lh = juce::jmax(1, h / kDownscale);
    auto low = sharp_.rescaled(lw, lh, juce::Graphics::highResamplingQuality);

    blurred_ = juce::Image(juce::Image::ARGB, lw, lh, true);
    juce::ImageConvolutionKernel kernel(7);
    kernel.createGaussianBlur(3.0f);
    kernel.applyToImage(blurred_, low, blurred_.getBounds());
}

void GlassBackground::paint(juce::Graphics& g)
{
    if (sharp_.isValid())
        g.drawImageAt(sharp_, 0, 0);
    else
        g.fillAll(palette_.bgApp);
}

} // namespace veyra::ui
