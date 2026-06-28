#pragma once

// Live metering channel: the service's loopback capture publishes a compact
// spectrum + VU/peak/clip snapshot here; the UI visualizer reads it to draw the
// real output instead of placeholder animation. Sequence-locked (single writer,
// wait-free reader) like the parameter blocks. When the service isn't running
// the UI falls back to its idle animation.

#include <atomic>
#include <cstdint>

namespace veyra::ipc {

// Global\ prefix — see kSharedTrackerName in TrackerData.h for rationale.
inline constexpr wchar_t kSharedAnalyzerName[] = L"Global\\VeyraAnalyzer_v1";
inline constexpr int      kAnalyzerBars = 48; // matches the visualizer bar count

struct VeyraAnalyzerPayload {
    float    vuL = 0.0f, vuR = 0.0f;     // RMS, 0..1
    float    peakL = 0.0f, peakR = 0.0f; // peak-hold, 0..1
    uint32_t clip = 0;
    float    bars[kAnalyzerBars] = {};   // normalised spectrum, 0..1
};

struct alignas(64) VeyraAnalyzerData {
    std::atomic<uint64_t> generation;
    VeyraAnalyzerPayload   payload;
    char _reserved[64 - (sizeof(std::atomic<uint64_t>) + sizeof(VeyraAnalyzerPayload)) % 64];
};

inline void publishAnalyzer(VeyraAnalyzerData* shm, const VeyraAnalyzerPayload& values) noexcept
{
    const uint64_t g = shm->generation.load(std::memory_order_relaxed);
    shm->generation.store(g + 1, std::memory_order_release);
    std::atomic_thread_fence(std::memory_order_release);
    shm->payload = values;
    std::atomic_thread_fence(std::memory_order_release);
    shm->generation.store(g + 2, std::memory_order_release);
}

inline bool readAnalyzer(const VeyraAnalyzerData* shm, VeyraAnalyzerPayload& out,
                         int maxTries = 8) noexcept
{
    for (int attempt = 0; attempt < maxTries; ++attempt)
    {
        const uint64_t g1 = shm->generation.load(std::memory_order_acquire);
        if (g1 & 1u)
            continue;
        std::atomic_thread_fence(std::memory_order_acquire);
        VeyraAnalyzerPayload snapshot = shm->payload;
        std::atomic_thread_fence(std::memory_order_acquire);
        const uint64_t g2 = shm->generation.load(std::memory_order_acquire);
        if (g1 == g2)
        {
            out = snapshot;
            return true;
        }
    }
    return false;
}

} // namespace veyra::ipc
