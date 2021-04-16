#ifndef CONTAINER_UTILS_HPP
#define CONTAINER_UTILS_HPP

#include <cstdint>
#include <new>

#include "BellLogging.hpp"
#include "Engine/Allocators.hpp"


template<typename T, uint32_t N>
class StaticGrowableBuffer
{
public:
    StaticGrowableBuffer() :
    mSize{0} {}

    uint32_t size() const
    {
	return mSize;
    }

    void push_back(const T& val)
    {
        BELL_ASSERT(mSize < N, "Need to increase buffer size")
        mArray[mSize++] = val;
    }

    void clear()
    {
        mSize = 0;
    }

    T* data()
    {
        return &mArray[0];
    }

    const T* data() const
    {
        return &mArray[0];
    }

    T* begin()
    {
        return &mArray[0];
    }

    const T* begin() const
    {
        return &mArray[0];
    }

    T* end()
    {
        return &mArray[mSize];
    }

    const T* end() const
    {
        return &mArray[mSize];
    }

private:
    T mArray[N];
    uint32_t mSize;
};

template<typename T>
class Array
{
public:

    Array(const uint64_t count, RAIISlabAllocator& alloc)
    {
        mBacking = static_cast<T*>(alloc.allocate(count * sizeof(T), alignof(T)));
        mSize = count;
        mNext = 0;
        mAllocator = nullptr;

        BELL_ASSERT(mBacking, "Allocator is exhausted")
    }

    Array(const uint64_t count, SlabAllocator& alloc)
    {
        mBacking = static_cast<T*>(alloc.allocate(count * sizeof(T), alignof(T)));
        mSize = count;
        mNext = 0;
        mAllocator = nullptr;

        BELL_ASSERT(mBacking, "Allocator is exhausted")
    }

    Array(const uint64_t count, std::pmr::memory_resource* alloc)
    {
        mBacking = static_cast<T*>(alloc->allocate(count * sizeof(T), alignof(T)));
        mSize = count;
        mNext = 0;
        mAllocator = alloc;

        BELL_ASSERT(mBacking, "Allocator is exhausted")
    }

    ~Array()
    {
        // not using a frame/slab allocator so need to manually free memory.
        if(mAllocator)
        {
            mAllocator->deallocate(mBacking, mSize * sizeof(T), alignof(T));
        }
    }

    T* data()
    {
        return mBacking;
    }

    const T* data() const
    {
        return mBacking;
    }

    uint64_t getMaxSize() const
    {
        return mSize;
    };

    uint64_t getSize() const
    {
        return mNext;
    }

    T* begin()
    {
        return mBacking;
    }

    T* end()
    {
        return mBacking + mSize;
    }

    const T* begin() const
    {
        return mBacking;
    }

    const T* end() const
    {
        return mBacking + mSize;
    }

    T& back()
    {
        return mBacking[mNext - 1];
    }

    const T& back() const
    {
        return mBacking[mNext - 1];
    }

    void push_back(const T& val)
    {
        BELL_ASSERT(mNext < mSize, "Array out of space")
        new (&mBacking[mNext]) T(val);

        ++mNext;
    }

    void push_back(T&& val)
    {
        BELL_ASSERT(mNext < mSize, "Array out of space")
        new (&mBacking[mNext]) T(std::move(val));

        ++mNext;
    }

    T& operator[](const uint64_t i)
    {
        BELL_ASSERT(i < mNext, "index out of bounds")

        return mBacking[i];
    }

    const T& operator[](const uint64_t i) const
    {
        BELL_ASSERT(i < mNext, "index out of bounds")

        return mBacking[i];
    }

private:

    T* mBacking;
    uint64_t mSize;
    uint64_t mNext;
    std::pmr::memory_resource* mAllocator;
};


#endif
