#include "Allocators.hpp"
#include "Core/BellLogging.hpp"

#include <cmath>
#include <cstring>

namespace
{
    uintptr_t nextAlignedAddress(const uintptr_t address, const uint64_t alignment) // Alignment must be a power of 2!
    {
        return (address + alignment - 1) & ~(alignment - 1);
    }
}

SlabAllocator::SlabAllocator(const uint64_t startingSize) :
    mResource{nullptr},
    mOffset{0},
    mSize{startingSize}
{
    mResource = static_cast<unsigned char*>(malloc(mSize));
}


SlabAllocator::~SlabAllocator()
{
    free(mResource);
}


SlabAllocator& SlabAllocator::operator=(SlabAllocator&& other) noexcept
{
    mResource = other.mResource;
    mOffset = other.mOffset;
    mSize = other.mSize;

    other.mResource = nullptr;

    return *this;
}


SlabAllocator::SlabAllocator(SlabAllocator&& other) noexcept
{
    mResource = other.mResource;
    mOffset = other.mOffset;
    mSize = other.mSize;

    other.mResource = nullptr;
}

void* SlabAllocator::allocate(size_t size, size_t alignment)
{
    unsigned char* nextAddress = reinterpret_cast<unsigned char*>(nextAlignedAddress( uintptr_t(mResource), alignment));

    void* address = nullptr;
    if(nextAddress + size <= mResource + mSize)
    {
        address = nextAddress;
        mOffset = (nextAddress + size) - mResource;
    }

    return address;
}


bool SlabAllocator::is_equal(const SlabAllocator& other) const
{
    return mResource == other.mResource;
}


RAIISlabAllocator::RAIISlabAllocator(SlabAllocator& allocator) :
    mStartOffset(allocator.mOffset),
    mSize(allocator.mSize),
    mOffset(allocator.mOffset),
    mResource(allocator.mResource) {}


RAIISlabAllocator::~RAIISlabAllocator()
{
    mOffset = mStartOffset;
}


RAIISlabAllocator& RAIISlabAllocator::operator=(RAIISlabAllocator&& other) noexcept
{
    mStartOffset = other.mStartOffset;
    mSize = other.mStartOffset;
    mResource = other.mResource;
    mOffset = other.mOffset;

    other.mResource = nullptr;

    return *this;
}


RAIISlabAllocator::RAIISlabAllocator(RAIISlabAllocator&& other) noexcept :
    mOffset(other.mOffset)
{
    mStartOffset = other.mStartOffset;
    mSize = other.mStartOffset;
    mResource = other.mResource;

    other.mResource = nullptr;
}



void* RAIISlabAllocator::allocate(size_t size, size_t alignment)
{
    unsigned char* nextAddress = reinterpret_cast<unsigned char*>(nextAlignedAddress( uintptr_t(mResource), alignment));

    void* address = nullptr;
    if(nextAddress + size <= mResource + mSize)
    {
        address = nextAddress;
        mOffset = (nextAddress + size) - mResource;
    }

    return address;
}


bool RAIISlabAllocator::is_equal(const RAIISlabAllocator& other) const
{
    return mResource == other.mResource;
}


PoolAllocator::PoolAllocator(const uint64_t size, const uint64_t count) :
        mCount(count),
        mSize(size)
{
    mResource = static_cast<unsigned char*>(malloc(mCount * mSize));
    mActiveSlots = static_cast<uint64_t*>(malloc(((mCount + 63) / 64) * sizeof(uint64_t)));
    std::memset(mActiveSlots, 1, ((mCount + 63) / 64) * sizeof(uint64_t));
}


PoolAllocator::~PoolAllocator()
{
    free(mResource);
    free(mActiveSlots);
}


PoolAllocator& PoolAllocator::operator=(PoolAllocator&& other) noexcept
{
    mResource = other.mResource;
    mCount = other.mCount;
    mSize = other.mSize;
    mActiveSlots = other.mActiveSlots;

    other.mResource = nullptr;
    other.mCount = 0;
    other.mSize = 0;
    other.mActiveSlots = nullptr;

    return *this;
}


PoolAllocator::PoolAllocator(PoolAllocator&& other) noexcept
{
    mResource = other.mResource;
    mCount = other.mCount;
    mSize = other.mSize;
    mActiveSlots = other.mActiveSlots;

    other.mResource = nullptr;
    other.mCount = 0;
    other.mSize = 0;
    other.mActiveSlots = nullptr;
}


void* PoolAllocator::allocate()
{
    const uint64_t slotCount = (mCount + 63) / 64;
    for(uint64_t i = 0; i < slotCount; ++i)
    {
        if(mActiveSlots[i] == 0)
            continue;

#ifdef _MSC_VER
        unsigned long firstFreeIndex;
        _BitScanForward64(&firstFreeIndex, mActiveSlots[i]);
#else
        const uint64_t firstFreeIndex = __builtin_ctzll(mActiveSlots[i]);
#endif

        mActiveSlots[i] &= ~(1ULL << firstFreeIndex);
        return mResource + ((i * 64) + firstFreeIndex) * mSize;
    }

    return nullptr;
}


void PoolAllocator::deallocate(const void* p)
{
    const uint64_t index = (static_cast<const unsigned char*>(p) - mResource) / mSize;
    const uint64_t slotIndex = index / 64;
    const uint64_t slotOffset = index % 64;

    mActiveSlots[slotIndex] |= (1ULL << slotOffset);
}


Allocator::Allocator() :
    mAllocatorPools{{1ULL << 3, 10 * 1024 * 1024},
                    {1ULL << 4, 10 * 1024 * 1024},
                    {1ULL << 5, 10 * 1024},
                    {1ULL << 6, 10 * 1024},
                    {1ULL << 7, 10 * 1024},
                    {1ULL << 8, 10 * 1024},
                    {1ULL << 9, 10 * 1024},
                    {1ULL << 10, 10 * 1024}}
{
}


Allocator& Allocator::operator=(Allocator&& other)
{
    std::memcpy(mAllocatorPools, other.mAllocatorPools, sizeof(PoolAllocator) * 8);

    return *this;
}


Allocator::Allocator(Allocator&& other) :
        mAllocatorPools{std::move(other.mAllocatorPools[0]),
                        std::move(other.mAllocatorPools[1]),
                        std::move(other.mAllocatorPools[2]),
                        std::move(other.mAllocatorPools[3]),
                        std::move(other.mAllocatorPools[4]),
                        std::move(other.mAllocatorPools[5]),
                        std::move(other.mAllocatorPools[6]),
                        std::move(other.mAllocatorPools[7])}
{}


void* Allocator::do_allocate(const uint64_t size, const uint64_t alignment)
{
    uint32_t pool = std::log2(size);

    // Min pool size is 1 << 3.
    pool = std::max(pool, 3u) - 3u;
    BELL_ASSERT(pool < 8, "larger pools need adding")

    void* addr = mAllocatorPools[pool].allocate();

    return  reinterpret_cast<void*>(nextAlignedAddress( uintptr_t(addr), alignment));
}


void Allocator::do_deallocate(void* p, std::size_t size, std::size_t)
{
    uint32_t pool = std::log2(size);

    // Min pool size is 1 << 3.
    pool = std::max(pool, 3u) - 3u;
    mAllocatorPools[pool].deallocate(p);
}


bool Allocator::do_is_equal(const std::pmr::memory_resource&) const noexcept
{
    return true;
}
