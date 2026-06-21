#pragma once

// Scene-aware content detector (spec §16). v1 is a deterministic, rule-based
// classifier over short-term spectral features — no ML model — exactly the
// "stub the interface, ship without inference" intent. A future learned model can
// drop in behind classify(). Pure + unit-testable.

#include <algorithm>

namespace veyra::dsp {

enum class Scene { Silence, Music, Movie, Game, Voice };

struct SceneFeatures {
    float rms      = 0.0f; // broadband level (linear)
    float lowFrac  = 0.0f; // fraction of energy < ~250 Hz
    float midFrac  = 0.0f; // ~250 Hz .. 2 kHz
    float highFrac = 0.0f; // > ~2 kHz
    float flux     = 0.0f; // spectral flux / transient activity 0..1
    float centroid = 0.0f; // brightness 0..1
};

class SceneDetector {
public:
    // Instantaneous classification from one feature frame.
    Scene classify(const SceneFeatures& f) const noexcept
    {
        if (f.rms < 0.005f)
            return Scene::Silence;
        // Voice/dialogue: mid-dominant, stable (low flux), not bright.
        if (f.midFrac > 0.50f && f.flux < 0.12f && f.centroid < 0.55f)
            return Scene::Voice;
        // Game: high transient activity + meaningful high-band content.
        if (f.flux > 0.25f && f.highFrac > 0.18f)
            return Scene::Game;
        // Movie: wide spectrum with strong lows (effects) + dialogue mids.
        if (f.lowFrac > 0.35f && f.midFrac > 0.22f)
            return Scene::Movie;
        return Scene::Music;
    }

    // Hysteresis: only switch after `holdFrames` consecutive agreeing frames, so
    // the active scene never flickers (the spec wants slow, never abrupt).
    Scene update(const SceneFeatures& f, int holdFrames = 8) noexcept
    {
        const Scene s = classify(f);
        if (s == candidate_) { ++streak_; }
        else                 { candidate_ = s; streak_ = 1; }
        if (streak_ >= holdFrames)
            stable_ = candidate_;
        return stable_;
    }

    Scene stable() const noexcept { return stable_; }

private:
    Scene candidate_ = Scene::Silence;
    Scene stable_    = Scene::Silence;
    int   streak_    = 0;
};

} // namespace veyra::dsp
