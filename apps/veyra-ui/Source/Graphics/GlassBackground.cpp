#include "Graphics/GlassBackground.h"

namespace veyra::ui {

void GlassBackground::ensureNoise()
{
    if (noise_.isValid())
        return;

    constexpr int n = 128;
    noise_ = juce::Image(juce::Image::ARGB, n, n, true);
    juce::Random rng(0x5EED);
    for (int yy = 0; yy < n; ++yy)
        for (int xx = 0; xx < n; ++xx)
        {
            const float v = rng.nextFloat();
            noise_.setPixelAt(xx, yy, juce::Colours::white.withAlpha(v * 0.04f));
        }
}

void GlassBackground::paint(juce::Graphics& g)
{
    const auto b = getLocalBounds().toFloat();
    g.fillAll(palette_.bgApp);

    struct Blob { float rx, ry, rr; juce::Colour c; };
    const Blob blobs[] = {
        {0.18f, 0.16f, 0.55f, palette_.accentPrimary.withAlpha(0.16f)},
        {0.85f, 0.30f, 0.45f, palette_.accentSecondary.withAlpha(0.10f)},
        {0.65f, 0.90f, 0.60f, palette_.accentPrimary.withAlpha(0.12f)},
        {0.05f, 0.95f, 0.40f, palette_.info.withAlpha(0.08f)},
    };

    for (const auto& blob : blobs)
    {
        const float cx = b.getWidth() * blob.rx;
        const float cy = b.getHeight() * blob.ry;
        const float rad = juce::jmax(b.getWidth(), b.getHeight()) * blob.rr;

        juce::ColourGradient grad(blob.c, cx, cy,
                                  blob.c.withAlpha(0.0f), cx + rad, cy, true);
        g.setGradientFill(grad);
        g.fillEllipse(cx - rad, cy - rad, rad * 2.0f, rad * 2.0f);
    }

    ensureNoise();
    g.setOpacity(1.0f);
    g.setTiledImageFill(noise_, 0, 0, 1.0f);
    g.fillRect(getLocalBounds());
}

} // namespace veyra::ui
