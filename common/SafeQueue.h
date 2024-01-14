#pragma once

#include <assert.h>
#include <atomic>
#include <pthread.h>
#include <vector>

template <typename T> class SafeQueue
{
public:
    SafeQueue(std::size_t numElements) : store(numElements, T())
    {
    }

    T *GetNextWriteTo()
    {
        return &store[nextWriteIndex];
    }

    void UpdateWriteIndex()
    {
        nextWriteIndex = (nextWriteIndex + 1) % store.size();
        numElements++;
    }

    T const *GetNextRead() const
    {
        return (nextReadIndex == nextWriteIndex) ? nullptr : &store[nextReadIndex];
    }

    T *GetNextRead()
    {
        return (nextReadIndex == nextWriteIndex) ? nullptr : &store[nextReadIndex];
    }

    void UpdateReadIndex()
    {
        nextReadIndex = (nextReadIndex + 1) % store.size();
        assert(numElements != 0);
        numElements--;
    }

    std::size_t GetSize() const
    {
        return numElements;
    }

    SafeQueue(SafeQueue const &) = delete;
    SafeQueue(SafeQueue &&) = delete;

    SafeQueue &operator=(SafeQueue const &) = delete;
    SafeQueue &operator=(SafeQueue &&) = delete;

private:
    std::vector<T> store;
    std::atomic<std::size_t> nextWriteIndex = 0;
    std::atomic<std::size_t> nextReadIndex = 0;
    std::atomic<std::size_t> numElements = 0;
};
