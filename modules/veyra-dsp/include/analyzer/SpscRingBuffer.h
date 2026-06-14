#pragma once

// Lock-free single-producer / single-consumer ring buffer. The audio thread
// pushes analyzer frames; the UI thread pops them. One slot is always left
// empty to disambiguate full from empty, so usable capacity is Capacity-1.
// No locks, no allocation — safe for the real-time producer.

#include <array>
#include <atomic>
#include <cstddef>

namespace veyra::dsp {

template <typename T, std::size_t Capacity>
class SpscRingBuffer {
public:
    static_assert(Capacity >= 2, "Capacity must be at least 2");

    // Producer side. Returns false if the buffer is full (frame dropped).
    bool push(const T& item) noexcept
    {
        const std::size_t tail = tail_.load(std::memory_order_relaxed);
        const std::size_t next = increment(tail);
        if (next == head_.load(std::memory_order_acquire))
            return false;
        buffer_[tail] = item;
        tail_.store(next, std::memory_order_release);
        return true;
    }

    // Consumer side. Returns false if the buffer is empty.
    bool pop(T& out) noexcept
    {
        const std::size_t head = head_.load(std::memory_order_relaxed);
        if (head == tail_.load(std::memory_order_acquire))
            return false;
        out = buffer_[head];
        head_.store(increment(head), std::memory_order_release);
        return true;
    }

    bool empty() const noexcept
    {
        return head_.load(std::memory_order_acquire) ==
               tail_.load(std::memory_order_acquire);
    }

private:
    static std::size_t increment(std::size_t i) noexcept { return (i + 1) % Capacity; }

    std::array<T, Capacity> buffer_{};
    std::atomic<std::size_t> head_{0};
    std::atomic<std::size_t> tail_{0};
};

} // namespace veyra::dsp
