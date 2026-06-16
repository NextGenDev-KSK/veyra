#pragma once

// Shared value types for the Sound Tracker (Gamer Mode): the classes of sound
// it recognises, the per-frame audio feature vector, and the events it emits to
// the overlay (direction + intensity of a detected sound).

namespace veyra::dsp {

enum class SoundClass {
    None = 0,
    Footstep,
    Gunshot,
    Voice,
    Other,
};

struct AudioFeatures {
    float rms = 0.0f;        // broadband level (linear)
    float lowEnergy = 0.0f;  // < ~250 Hz
    float midEnergy = 0.0f;  // ~250 Hz .. 2 kHz (derived)
    float highEnergy = 0.0f; // > ~2 kHz
    float flux = 0.0f;       // positive level change vs previous frame (onset)
    float centroid = 0.0f;   // brightness 0..1 (highEnergy / total)
};

struct TrackerEvent {
    SoundClass type = SoundClass::None;
    float azimuthDeg = 0.0f; // -90 (left) .. +90 (right)
    float intensity = 0.0f;  // linear level of the event
    float confidence = 0.0f; // 0..1
};

} // namespace veyra::dsp
