#pragma once

// The shared-memory contract between the service (single writer) and the APO
// (reader, inside audiodg.exe's real-time thread).
//
// Consistency uses a sequence lock: the writer bumps 'generation' to an odd
// value before writing the payload and to the next even value after. The reader
// retries while it sees an odd generation or a changed generation across the
// copy. This is wait-free for the reader and never blocks the audio thread.

#include <atomic>
#include <cstdint>
#include <cstring>

namespace veyra::ipc {

inline constexpr wchar_t kSharedParametersName[] = L"Local\\VeyraAPOParameters_v1";

// Plain-old-data payload (no atomics) so it can be copied by value. Bools are
// uint32 for a stable cross-module layout. Mirrors veyra::dsp::DspParameters;
// the APO maps between the two.
struct VeyraParamsPayload {
    uint32_t bypass = 0;
    uint32_t monoMode = 0;
    float    balance = 0.0f;
    float    eqBandsDb[10] = {};
    float    bassBoostDb = 0.0f;
    float    trebleDb = 0.0f;
    float    compressionAmount = 0.0f;
    float    exciterAmount = 0.0f;
    float    stereoWidth = 1.0f;
    float    volumeGain = 1.0f;
    float    crossfeedAmount = 0.0f;
    float    virtualizationAmount = 0.0f;
    float    nightModeAmount = 0.0f;
    uint32_t loudnessMatch = 0;
    float    loudnessTargetLufs = -14.0f;
    uint32_t equalLoudness = 0;
    uint32_t referenceMode = 0;
    float    limiterCeilingDb = -0.3f;
};

// Cache-line aligned so the seqlock counter doesn't false-share with anything.
struct alignas(64) VeyraSharedParameters {
    std::atomic<uint64_t> generation;
    VeyraParamsPayload     payload;
    // pad out to a multiple of the cache line.
    char _reserved[64 - (sizeof(std::atomic<uint64_t>) + sizeof(VeyraParamsPayload)) % 64];
};

// Writer side (service): publish a new payload atomically w.r.t. readers.
inline void publishParameters(VeyraSharedParameters* shm, const VeyraParamsPayload& values) noexcept
{
    const uint64_t g = shm->generation.load(std::memory_order_relaxed);
    shm->generation.store(g + 1, std::memory_order_release); // mark "writing" (odd)
    std::atomic_thread_fence(std::memory_order_release);
    shm->payload = values;
    std::atomic_thread_fence(std::memory_order_release);
    shm->generation.store(g + 2, std::memory_order_release); // done (even)
}

// Reader side (APO): copy a consistent snapshot. Returns false only if it could
// not get a stable read within a bounded number of tries (writer thrashing),
// in which case the caller should keep using its previous snapshot.
inline bool readParameters(const VeyraSharedParameters* shm, VeyraParamsPayload& out,
                           int maxTries = 8) noexcept
{
    for (int attempt = 0; attempt < maxTries; ++attempt)
    {
        const uint64_t g1 = shm->generation.load(std::memory_order_acquire);
        if (g1 & 1u)
            continue; // writer in progress
        std::atomic_thread_fence(std::memory_order_acquire);
        VeyraParamsPayload snapshot = shm->payload;
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

// --- Microphone (capture) chain ---------------------------------------------
// Separate shared block written by the service and read by the capture APO.

inline constexpr wchar_t kSharedMicParametersName[] = L"Local\\VeyraMicParameters_v1";

struct VeyraMicParamsPayload {
    uint32_t enabled = 1;
    float    highPassHz = 80.0f;
    float    noiseSuppression = 0.5f;
    float    compressionAmount = 0.3f;
    float    deEssAmount = 0.3f;
    float    presenceDb = 2.0f;
    float    outputGainDb = 0.0f;
    float    sideToneLevel = 0.0f;
    uint32_t agc = 0;
};

struct alignas(64) VeyraMicSharedParameters {
    std::atomic<uint64_t> generation;
    VeyraMicParamsPayload  payload;
    char _reserved[64 - (sizeof(std::atomic<uint64_t>) + sizeof(VeyraMicParamsPayload)) % 64];
};

inline void publishMicParameters(VeyraMicSharedParameters* shm,
                                 const VeyraMicParamsPayload& values) noexcept
{
    const uint64_t g = shm->generation.load(std::memory_order_relaxed);
    shm->generation.store(g + 1, std::memory_order_release);
    std::atomic_thread_fence(std::memory_order_release);
    shm->payload = values;
    std::atomic_thread_fence(std::memory_order_release);
    shm->generation.store(g + 2, std::memory_order_release);
}

inline bool readMicParameters(const VeyraMicSharedParameters* shm,
                              VeyraMicParamsPayload& out, int maxTries = 8) noexcept
{
    for (int attempt = 0; attempt < maxTries; ++attempt)
    {
        const uint64_t g1 = shm->generation.load(std::memory_order_acquire);
        if (g1 & 1u)
            continue;
        std::atomic_thread_fence(std::memory_order_acquire);
        VeyraMicParamsPayload snapshot = shm->payload;
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
