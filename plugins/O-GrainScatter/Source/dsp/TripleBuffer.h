#pragma once

#include <array>
#include <atomic>

// Lock-free single-producer single-consumer triple buffer.
// Writer and reader never contend on the same slot.
template <typename T>
class TripleBuffer
{
public:
    T& getWriteBuffer() { return buffers[static_cast<size_t> (writeIdx)]; }

    void publish()
    {
        writeIdx = middle.exchange (writeIdx, std::memory_order_acq_rel);
        newData.store (true, std::memory_order_release);
    }

    const T& read()
    {
        if (newData.exchange (false, std::memory_order_acq_rel))
            readIdx = middle.exchange (readIdx, std::memory_order_acq_rel);
        return buffers[static_cast<size_t> (readIdx)];
    }

    bool hasNewData() const { return newData.load (std::memory_order_acquire); }

private:
    std::array<T, 3> buffers {};
    int writeIdx = 0;
    std::atomic<int> middle { 1 };
    int readIdx = 2;
    std::atomic<bool> newData { false };
};
