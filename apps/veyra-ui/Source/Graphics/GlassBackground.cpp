#include "Graphics/GlassBackground.h"

namespace veyra::ui {

void GlassBackground::renderSharp(juce::Image& img) const
{
    juce::Graphics g(img);
    const auto b = juce::Rectangle<float>(0.0f, 0.0f, (float) img.getWidth(), (float) img.getHeight());

    g.fillAll(palette_.bgApp);

    struct Blob { float rx, ry, rr; juce::Colour c; };
    const Blob blobs[] = {
        {0.16f, 0.14f, 0.55f, palette_.accentPrimary.withAlpha(0.22f)},
        {0.88f, 0.26f, 0.45f, palette_.accentSecondary.withAlpha(0.12f)},
        {0.70f, 0.96f, 0.62f, palette_.accentPrimary.withAlpha(0.16f)},
        {0.04f, 0.98f, 0.42f, palette_.info.withAlpha(0.10f)},
    };
    for (const auto& blob : blobs)
    {
        const float cx = b.getWidth() * blob.rx;
        const float cy = b.getHeight() * blob.ry;
        const float rad = juce::jmax(b.getWidth(), b.getHeight()) * blob.rr;
        juce::ColourGradient grad(blob.c, cx, cy, blob.c.withAlpha(0.0f), cx + rad, cy, true);
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
