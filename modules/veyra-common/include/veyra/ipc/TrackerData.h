#pragma once

// Shared-memory channel carrying Sound Tracker detections from the producer
// (the analysis side that runs veyra::dsp::SoundTracker over a loopback capture)
// to the overlay that draws the radar. A lock-free ring of recent events with a
// monotonic write counter: the producer never blocks, and the overlay reads the
// events it hasn't seen yet (older ones are overwritten if it falls behind).

#include <atomic>
#include <cstdint>

namespace veyra::ipc {

inline constexpr wchar_t kSharedTrackerName[] = L"Local\\VeyraTracker_v1";

// Mirrors veyra::dsp::SoundClass as a stable wire value.
enum class TrackerEventType : int32_t {
    None = 0,
    Footstep,
    Gunshot,
    Voice,
    Other,
};

struct TrackerEventRecord {
    int32_t type = 0;          // TrackerEventType
    float   azimuthDeg = 0.0f; // -90 .. +90
    float   intensity = 0.0f;
    float   confidence = 0.0f;
    double  timestampSec = 0.0;
};

struct alignas(64) VeyraTrackerData {
    static constexpr int kCapacity = 32;
    std::atomic<uint64_t> writeCount; // total events ever written (monotonic)
    TrackerEventRecord    events[kCapacity];
};

// Producer: append one event. Wait-free; the slot is published by bumping the
// write counter with release ordering after the record is stored.
inline void writeTrackerEvent(VeyraTrackerData* shm, const TrackerEventRecord& rec) noexcept
{
    const uint64_t idx = shm->writeCount.load(std::memory_order_relaxed);
    shm->events[idx % VeyraTrackerData::kCapacity] = rec;
    std::atomic_thread_fence(std::memory_order_release);
    shm->writeCount.store(idx + 1, std::memory_order_release);
}

// Consumer: drain events newer than 'lastSeen' (updated in place) into 'out'
// (capacity outMax). Returns the number written. If the consumer fell more than
// kCapacity behind, it resynchronises to the oldest still-available event.
inline int readTrackerEvents(const VeyraTrackerData* shm, uint64_t& lastSeen,
                             TrackerEventRecord* out, int outMax) noexcept
{
    const uint64_t count = shm->writeCount.load(std::memory_order_acquire);
    std::atomic_thread_fence(std::memory_order_acquire);

    const uint64_t cap = static_cast<uint64_t>(VeyraTrackerData::kCapacity);
    uint64_t from = lastSeen;
    if (count > from + cap)
        from = count - cap; // we fell behind; skip overwritten events

    int n = 0;
    for (uint64_t i = from; i < count && n < outMax; ++i)
        out[n++] = shm->events[i % VeyraTrackerData::kCapacity];
    lastSeen = count;
    return n;
}

} // namespace veyra::ipc
