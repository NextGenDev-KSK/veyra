#include <catch2/catch.hpp>

#include "scene/SceneDetector.h"

using namespace veyra::dsp;

TEST_CASE("SceneDetector: classifies representative feature frames")
{
    SceneDetector d;

    CHECK(d.classify({0.001f, 0.3f, 0.4f, 0.3f, 0.05f, 0.4f}) == Scene::Silence); // near-silent

    // Voice: mid-dominant, stable, not bright.
    CHECK(d.classify({0.2f, 0.15f, 0.65f, 0.20f, 0.05f, 0.40f}) == Scene::Voice);

    // Game: transient-heavy + high-band energy.
    CHECK(d.classify({0.3f, 0.20f, 0.35f, 0.45f, 0.40f, 0.6f}) == Scene::Game);

    // Movie: strong lows + dialogue mids.
    CHECK(d.classify({0.3f, 0.45f, 0.30f, 0.25f, 0.10f, 0.4f}) == Scene::Movie);

    // Music: balanced/bright, moderate flux (falls through).
    CHECK(d.classify({0.3f, 0.30f, 0.30f, 0.40f, 0.15f, 0.6f}) == Scene::Music);
}

TEST_CASE("SceneDetector: hysteresis holds a scene until consistently changed")
{
    SceneDetector d;
    const SceneFeatures voice{0.2f, 0.15f, 0.65f, 0.20f, 0.05f, 0.40f};
    const SceneFeatures game {0.3f, 0.20f, 0.35f, 0.45f, 0.40f, 0.60f};

    for (int i = 0; i < 8; ++i) d.update(voice, 8);
    CHECK(d.stable() == Scene::Voice);

    // A few game frames shouldn't flip it immediately...
    d.update(game, 8); d.update(game, 8);
    CHECK(d.stable() == Scene::Voice);

    // ...but a sustained run does.
    for (int i = 0; i < 8; ++i) d.update(game, 8);
    CHECK(d.stable() == Scene::Game);
}
