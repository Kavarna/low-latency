#pragma once

#include "Logger.h"
#include <vector>

template <typename T> class MemoryPool final
{
public:
    explicit MemoryPool(std::size_t numElems) : store(numElems, {T(), true})
    {
        DCHECK_FATAL(reinterpret_cast<T *>(&store[0].object) == reinterpret_cast<T *>(&store[0]),
                     "The object should be first in the object block structure");
    }

    MemoryPool() = delete;
    MemoryPool(MemoryPool const &) = delete;
    MemoryPool(MemoryPool &&) = delete;

    MemoryPool &operator=(MemoryPool const &) = delete;
    MemoryPool &operator=(MemoryPool &&) = delete;

public:
    template <typename... Args> T *Allocate(Args... args)
    {
        auto obj_block = &(store[nextFreeIndex]);
        CHECK(obj_block->isFree, nullptr, "Expected free block at index ", nextFreeIndex);

        T *ret = &obj_block->object;
        ret = new (ret) T(std::forward<Args>(args)...);
        obj_block->isFree = false;

        UpdateNextFreeIndex();

        return ret;
    }

    void Deallocate(T *elem)
    {
        auto elemIndex = reinterpret_cast<ObjectBlock *>(elem) - &store[0];
        DCHECK_FATAL(elemIndex >= 0 && static_cast<size_t>(elemIndex) < store.size(),
                     "Element that is expected to be deleted is not within store range. Maybe was allocated with "
                     "another memory pool?")

        DCHECK_FATAL(&store[elemIndex].object == elem,
                     "Element passed to Deallocate doesn't coincide with the element in store")

        DCHECK_FATAL(!store[elemIndex].isFree, "Expected in use element");

        store[elemIndex].isFree = true;
    }

private:
    inline void UpdateNextFreeIndex()
    {
        const auto initialFreeIndex = nextFreeIndex;
        while (!store[nextFreeIndex].isFree)
        {
            nextFreeIndex++;
            if (nextFreeIndex == store.size()) [[unlikely]]
            {
                nextFreeIndex = 0;
            }
            if (nextFreeIndex == initialFreeIndex) [[unlikely]]
            {
                SHOWWARNING("Memory pool is full. Should remove some elements before attempting to allocate");
            }
        }
    }

private:
    struct ObjectBlock
    {
        T object;
        bool isFree = true;
    };

    std::vector<ObjectBlock> store;
    size_t nextFreeIndex = 0;
};
