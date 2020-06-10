#include "MemoryManager.hpp"
#include "VulkanRenderDevice.hpp"
#include "Core/Buffer.hpp"
#include "Core/BellLogging.hpp"

#include <vulkan/vulkan.hpp>

#include <vector>
#include <list>
#include <iostream>
#include <algorithm>


namespace
{
	uint64_t nextAlignedAddress(const uint64_t address, const uint64_t alignment) // Alignemt must be a power of 2!
	{
		return (address + alignment - 1) & ~(alignment - 1);
	}
}

bool operator==(const PoolFragment& lhs, const PoolFragment& rhs)
{
    return lhs.DeviceLocal == rhs.DeviceLocal
            && lhs.free == rhs.free
            && lhs.offset == rhs.offset
            && lhs.size == rhs.size;
}


MemoryManager::MemoryManager(RenderDevice* dev) : DeviceChild{dev}
{
    findPoolIndicies();

    AllocateDevicePool();
    AllocateHostMappablePool();
}


void MemoryManager::Destroy()
{
    FreeDevicePools();
    FreeHostMappablePools();
}


#if MEMORY_LOGGING
	void MemoryManager::dumpPools() const
	{
        std::cout << "Device local pools: \n";
        for(auto& pool : deviceLocalPools) {
            for(auto& fragment : pool) {
                std::cout << "fragment size: " << fragment.size << '\n'
                          << "fragment offset: " << fragment.offset << '\n'
                          << "fragment marked for merge: " << fragment.canBeMerged << '\n'
                          << "fragment free: " << fragment.free << "\n\n\n";
            }
        }

        std::cout << "Host Mappable pools: \n";
        for(auto& pool : hostMappablePools) {
            for(auto& fragment : pool) {
                std::cout << "fragment size: " << fragment.size << '\n'
                          << "fragment offset: " << fragment.offset << '\n'
                          << "fragment marked for merge: " << fragment.canBeMerged << '\n'
                          << "fragment free: " << fragment.free << "\n\n\n";
            }
        }
    }
#endif


void MemoryManager::findPoolIndicies()
{
    const vk::PhysicalDeviceMemoryProperties& memProps = static_cast<VulkanRenderDevice*>(getDevice())->getMemoryProperties();

    bool poolFound = false;
	for(uint32_t i = 0; i < memProps.memoryTypeCount; ++i)
	{
        if(memProps.memoryTypes[i].propertyFlags & vk::MemoryPropertyFlagBits::eDeviceLocal
           && !(memProps.memoryTypes[i].propertyFlags & vk::MemoryPropertyFlagBits::eHostCoherent))
        {
            mDeviceLocalPoolIndex = i; // just find the find pool that is device local
            mDeviceLocalHeapindex = memProps.memoryTypes[i].heapIndex;
            poolFound = true;
            break;
        }
    }

	if(!poolFound)
	{
        for(uint32_t i = 0; i < memProps.memoryTypeCount; ++i) {
            if(memProps.memoryTypes[i].propertyFlags & vk::MemoryPropertyFlagBits::eDeviceLocal)
            {
                BELL_LOG("having to use host coherint memory, probably a integrated GPU")

                mDeviceLocalPoolIndex = i; // just find the first pool that is device local
                mDeviceLocalHeapindex = memProps.memoryTypes[i].heapIndex;
                break;
            }
        }
    }

	for(uint32_t i = 0; i < memProps.memoryTypeCount; ++i)
	{
		if((memProps.memoryTypes[i].propertyFlags & vk::MemoryPropertyFlagBits::eHostCoherent)
           && (memProps.memoryTypes[i]).propertyFlags & vk::MemoryPropertyFlagBits::eHostVisible)
        {
			mHostMappablePoolIndex = i;
            mHostMapableHeapindex = memProps.memoryTypes[i].heapIndex;
			mHasHostCoherent = true;

            // Prefer coherent memory.
            break;
        }

		if((memProps.memoryTypes[i].propertyFlags & vk::MemoryPropertyFlagBits::eDeviceLocal)
		   && (memProps.memoryTypes[i]).propertyFlags & vk::MemoryPropertyFlagBits::eHostVisible)
		{
			mHostMappablePoolIndex = i;
            mHostMapableHeapindex = memProps.memoryTypes[i].heapIndex;
			mHasHostCoherent = false;
		}
    }

    
}


void MemoryManager::AllocateDevicePool()
{
    constexpr vk::DeviceSize poolSize =  256 * 1024 * 1024;
    vk::MemoryAllocateInfo allocInfo{ poolSize, static_cast<uint32_t>(mDeviceLocalPoolIndex)};

	mDeviceMemoryBackers.push_back({nullptr, static_cast<VulkanRenderDevice*>(getDevice())->allocateMemory(allocInfo)});

    std::list<PoolFragment> fragmentList(4);
    uint64_t offset = 0;
	for(auto& frag : fragmentList)
	{
        frag.free = true;
        frag.DeviceLocal = true;
        frag.size = poolSize / 4;
        frag.offset = offset;

        offset += frag.size;
    }

	mDeviceLocalPools.push_back(fragmentList);

    BELL_LOG("Allocated a memory pool")
}


void MemoryManager::AllocateHostMappablePool()
{
    VulkanRenderDevice* device = static_cast<VulkanRenderDevice*>(getDevice());

    const vk::PhysicalDeviceMemoryProperties& memProps = device->getMemoryProperties();
	const vk::DeviceSize poolSize = std::min(256ULL * 1024ULL * 1024ULL, memProps.memoryHeaps[mHostMapableHeapindex].size);
	vk::MemoryAllocateInfo allocInfo{poolSize, static_cast<uint32_t>(mHostMappablePoolIndex)};

	// Map the entire allocation on creation for persistent mapping.
	const vk::DeviceMemory backingMemory = device->allocateMemory(allocInfo);
	void* baseAddress = device->mapMemory(backingMemory, poolSize, 0);

	mHostMappableMemoryBackers.push_back({baseAddress, backingMemory});

    std::list<PoolFragment> fragmentList(4);
    uint64_t offset = 0;
	for(auto& frag : fragmentList)
	{
        frag.free = true;
        frag.DeviceLocal = true;
        frag.size = poolSize / 4;
        frag.offset = offset;

        offset += frag.size;
    }

	mHostMappablePools.push_back(fragmentList);

    BELL_LOG("Allocated a host mappable memory pool")
}


void MemoryManager::FreeDevicePools()
{
	for(auto& frags : mDeviceMemoryBackers)
	{
		static_cast<VulkanRenderDevice*>(getDevice())->freeMemory(frags.mBackingMemory);
    }
}


void MemoryManager::FreeHostMappablePools()
{
	for(auto& frags : mHostMappableMemoryBackers)
	{ // we assume that all has been unmapped
		static_cast<VulkanRenderDevice*>(getDevice())->freeMemory(frags.mBackingMemory);
    }
}


void MemoryManager::MergeFreePools()
{
	MergePool(mDeviceLocalPools);
	MergePool(mHostMappablePools);
}


void MemoryManager::MergePool(std::vector<std::list<PoolFragment> > &pools)
{ // this is expensive on a list so only call when really needed
	for(auto& pool : pools)
	{
        // loop forwards through all the fragments and mark any free fragments ajasent to a free fragment as can be merged
#if MEMORY_LOGGING
        const auto freeFragmentsPre = std::count_if(pool.begin(), pool.end(), [](auto& fragment){ return fragment.free; });
        std::cout << "number of free fragments pre merge: " << freeFragmentsPre << '\n';
#endif
		bool fragmentFree = false;
		for(auto& fragment : pool)
		{
            
			if(fragmentFree && fragment.free)
			{
                fragment.canBeMerged = true;
            }

			if(fragment.free)
			{
                fragmentFree = true;
			}
			else
			{
                fragmentFree = false;
            }
        }

        // Now loop backwards over the list and merge all fragments marked as can be merged
        uint64_t sizeToAdd = 0;
        bool fragmentMerged = false;
        std::vector<std::list<PoolFragment>::reverse_iterator> fragmentsToRemove;
		for(auto fragment = pool.rbegin(); fragment != pool.rend(); ++fragment)
		{
			if(fragmentMerged)
			{
                fragment->size += sizeToAdd;
                sizeToAdd = 0;
                fragmentMerged = false;
            }

			if(fragment->canBeMerged)
			{
                sizeToAdd = fragment->size;
                fragmentsToRemove.push_back(fragment);
                fragmentMerged = true;
            }
        }

        // One final loop over all the fragments to be removed in reverse to avoid a use after free
		for(auto iter = fragmentsToRemove.rbegin(); iter != fragmentsToRemove.rend(); ++iter)
		{
            pool.remove(**iter);
        }
#if MEMORY_LOGGING
        const auto freeFragmentsPost = std::count_if(pool.begin(), pool.end(), [](auto& fragment){ return fragment.free; });
        std::cout << "number of free fragments post merge: " << freeFragmentsPost << '\n';
#endif
    }
}


Allocation MemoryManager::AttemptToAllocate(const uint64_t size, const unsigned long allignment, const bool hostMappable, const std::string& name)
{
	auto& memPools = hostMappable ? mHostMappablePools : mDeviceLocalPools;

    uint32_t poolNum = 0;
    for(auto& pool : memPools) {
        for(auto fragIter = pool.begin(); fragIter != pool.end(); ++fragIter) {
            PoolFragment frag = *fragIter;

			uint64_t alignedAddress = nextAlignedAddress(frag.offset, allignment);
			const uint64_t alignedOffset = alignedAddress - frag.offset;

			if(frag.free && frag.size >= size + alignedOffset)
			{
                Allocation alloc;
                alloc.offset = alignedAddress;
                alloc.fragOffset = frag.offset;
                alloc.hostMappable = hostMappable;
                alloc.deviceLocal = true;
                alloc.pool = poolNum;
                alloc.size = size;

                if(size + alignedOffset < frag.size)
                { // Split the block.
                    PoolFragment fragToInsert;
                    fragToInsert.DeviceLocal = true;
                    fragToInsert.free = true;
                    fragToInsert.offset = alignedAddress + size;
                    fragToInsert.size = frag.size - (size + alignedOffset);

                    fragIter->size = size + alignedOffset;

					// Make sure we insert the split fragment infront of the allocated one
					// otherwise fragment merging won't work correctly.
					auto inFrontIt = fragIter;
					++inFrontIt;
                    pool.insert(inFrontIt, fragToInsert);
                }
                fragIter->free = false;
#ifndef NDEBUG
                fragIter->name = name;
#endif
                return alloc;
            }
        }
        ++poolNum;
    }

    Allocation alloc;
    alloc.size = 0; // signify that the allocation failed
    return alloc;
}


Allocation MemoryManager::Allocate(const uint64_t size, const unsigned long allignment,  const bool hostMappable, const std::string &name)
{

    Allocation alloc = AttemptToAllocate(size, allignment, hostMappable, name);
    if(alloc.size != 0) return alloc;

    MergeFreePools(); // if we failed to allocate, perform a defrag of the pools and try again.
    alloc = AttemptToAllocate(size, allignment, hostMappable, name);
    if(alloc.size != 0)
        return alloc;

	if(hostMappable)
	{
        AllocateHostMappablePool();
    }
    else
	{
        AllocateDevicePool();
    }
    MergeFreePools();
    return AttemptToAllocate(size, allignment, hostMappable, name); // should succeed or we are out of memory :(
}


void MemoryManager::Free(Allocation alloc)
{
	auto& pools = alloc.hostMappable ? mHostMappablePools : mDeviceLocalPools ;
    auto& pool  = pools[alloc.pool];

    for(auto& frag : pool) {
		if(alloc.fragOffset == frag.offset)
		{
            frag.free = true;
        }
    }
}


void MemoryManager::BindBuffer(vk::Buffer &buffer, const Allocation& alloc)
{
	const std::vector<MappableMemoryInfo>& pools = alloc.hostMappable ? mHostMappableMemoryBackers : mDeviceMemoryBackers ;

	static_cast<VulkanRenderDevice*>(getDevice())->bindBufferMemory(buffer, pools[alloc.pool].mBackingMemory, alloc.offset);
}


void MemoryManager::BindImage(vk::Image &image, const Allocation& alloc)
{
    const std::vector<MappableMemoryInfo>& pools = alloc.hostMappable ? mHostMappableMemoryBackers : mDeviceMemoryBackers;

    static_cast<VulkanRenderDevice*>(getDevice())->bindImageMemory(image, pools[alloc.pool].mBackingMemory, alloc.offset);
}


void* MemoryManager::MapAllocation(const MapInfo& info, const Allocation& alloc)
{	
	BELL_ASSERT(alloc.hostMappable, "Attempting to map non mappable memory")

	const std::vector<MappableMemoryInfo>& pools = alloc.hostMappable ? mHostMappableMemoryBackers : mDeviceMemoryBackers;

	return static_cast<void*>(static_cast<char*>(pools[alloc.pool].mBaseAddress) + alloc.offset + info.mOffset);
}


void MemoryManager::UnMapAllocation(const MapInfo &info, const Allocation& alloc)
{
	const std::vector<MappableMemoryInfo>& pools = alloc.hostMappable ? mHostMappableMemoryBackers : mDeviceMemoryBackers;

	if(writeMapsNeedFlushing())
	{
		vk::MappedMemoryRange range{};
		range.setMemory(pools[alloc.pool].mBackingMemory);
		range.setSize(info.mSize);
		range.setOffset(alloc.offset + info.mOffset);

		static_cast<VulkanRenderDevice*>(getDevice())->flushMemoryRange(range);
	}
}
