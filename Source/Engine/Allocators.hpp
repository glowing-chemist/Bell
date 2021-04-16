#ifndef BELL_CPU_ALLOCATORS_HPP
#define BELL_CPU_ALLOCATORS_HPP

#include <cstdint>
#include <cstdlib>
#include <memory_resource>


class SlabAllocator
{
public:
    friend class RAIISlabAllocator;

    SlabAllocator(const uint64_t startingSize);
    ~SlabAllocator();

    SlabAllocator& operator=(const SlabAllocator&) noexcept = delete;
    SlabAllocator(const SlabAllocator&) noexcept = delete;

    SlabAllocator& operator=(SlabAllocator&&) noexcept;
    SlabAllocator(SlabAllocator&&) noexcept;

    void reset()
    {
        mOffset = 0;
    }

    void* allocate(size_t size, size_t alignment);
    bool is_equal(const SlabAllocator& other) const;

private:

    unsigned char* mResource;
    uint64_t mOffset;
    uint64_t mSize;
};


class RAIISlabAllocator
{
public:

    RAIISlabAllocator(SlabAllocator&);
    ~RAIISlabAllocator();

    RAIISlabAllocator& operator=(const RAIISlabAllocator&) noexcept = delete;
    RAIISlabAllocator(const RAIISlabAllocator&) noexcept = delete;

    RAIISlabAllocator& operator=(RAIISlabAllocator&&) noexcept;
    RAIISlabAllocator(RAIISlabAllocator&&) noexcept;


    void* allocate(size_t size, size_t alignment);
    bool is_equal(const RAIISlabAllocator& other) const;

private:

    uint64_t mStartOffset;
    uint64_t mSize;
    uint64_t& mOffset;
    unsigned char* mResource;
};


// Pool allocators
class PoolAllocator
{
public:

    PoolAllocator(const uint64_t size, const uint64_t count);

    ~PoolAllocator();

    PoolAllocator& operator=(const PoolAllocator&) noexcept = delete;
    PoolAllocator(const PoolAllocator&) noexcept = delete;

    PoolAllocator& operator=(PoolAllocator&&) noexcept;
    PoolAllocator(PoolAllocator&&) noexcept;

    void* allocate();

    void deallocate(const void* p);

private:

    unsigned char* mResource;
    uint64_t mCount;
    uint64_t mSize;
    uint64_t* mActiveSlots;
};


template<typename T>
class TypedPoolAllocator
{
public:

    TypedPoolAllocator(const uint64_t count) : mPool(sizeof(T), count) {}
    ~TypedPoolAllocator() = default;

    TypedPoolAllocator& operator=(const TypedPoolAllocator&) noexcept = delete;
    TypedPoolAllocator(const TypedPoolAllocator&) noexcept = delete;

    TypedPoolAllocator& operator=(TypedPoolAllocator&& other) noexcept
    {
        mPool = other.mPool;

        return *this;
    }

    TypedPoolAllocator(TypedPoolAllocator&& other) noexcept : mPool(other.mPool) {}

    T* allocate()
    {
        return static_cast<T*>(mPool.allocate());
    }

    void deallocate(const T* p)
    {
        mPool.deallocate(p);
    }

private:

    PoolAllocator mPool;
};


// General allocators
class Allocator : public std::pmr::memory_resource
{
public:
    Allocator();
    ~Allocator() = default;

    Allocator& operator=(const Allocator&) = delete;
    Allocator(const Allocator&) = delete;

    Allocator& operator=(Allocator&&);
    Allocator(Allocator&&);

private:

    virtual void* do_allocate(const uint64_t size, const uint64_t alignment) final;
    virtual void do_deallocate(void* p, std::size_t bytes, std::size_t alignment) final;
    virtual bool do_is_equal(const std::pmr::memory_resource&) const noexcept final;

    // Pools for various power of 2 pools.
    PoolAllocator mAllocatorPools[8];
};

#endif