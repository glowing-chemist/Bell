#ifndef MemoryManager_HPP
#define MemoryManager_HPP

#include "Core/DeviceChild.hpp"

#include <vulkan/vulkan.hpp>

#include <vector>
#include <list>

#define MEMORY_LOGGING 0

struct MapInfo;

struct PoolFragment
{
    friend class MemoryManager; // to allow this to be an opaque handle that only the memory manager can use
    friend bool operator==(const PoolFragment&, const PoolFragment&);
#ifndef NDEBUG
    std::string name;
#endif
private:
    uint64_t offset;
    uint64_t size;
    bool DeviceLocal;
    bool free;
    bool canBeMerged = false;
};

struct Allocation
{
    friend class MemoryManager;
private:
    uint64_t size;
    uint64_t fragOffset;
    uint64_t offset;
    uint32_t pool; // for if we end up allocating more than one pool
    bool deviceLocal;
    bool hostMappable;
};


// This class will be used for keeping track of GPU allocations for buffers and
// images. this will be done by maintaing 2 pools of gpu memory, one device local
// and the other host mappable. allocations will be handles a opaque types that
// the caller will keep track of.
class MemoryManager : public DeviceChild
{
public:
	explicit MemoryManager(RenderDevice*);

    Allocation Allocate(const uint64_t size, const unsigned long allignment, const bool hostMappable, const std::string& name = "");
    void       Free(Allocation alloc);

    void       BindImage(vk::Image& image, const Allocation& alloc);
    void       BindBuffer(vk::Buffer& buffer, const Allocation& alloc);

	void*	   MapAllocation(const MapInfo& info, const Allocation& alloc);
	void	   UnMapAllocation(const MapInfo& info, const Allocation& alloc);

    void       Destroy();

	bool	   writeMapsNeedFlushing() const
				{ return !mHasHostCoherent; }

#if MEMORY_LOGGING
    void dumpPools() const;
#endif

private:
    void MergeFreePools();
    void MergePool(std::vector<std::list<PoolFragment>>& pools);

    Allocation AttemptToAllocate(const uint64_t size, const unsigned long allignment, const bool deviceLocal, const std::string &name);

    void AllocateDevicePool();
    void AllocateHostMappablePool();

    void FreeDevicePools();
    void FreeHostMappablePools();

    void findPoolIndicies();

    uint32_t mDeviceLocalPoolIndex;
    uint32_t mHostMappablePoolIndex;
	bool mHasHostCoherent;

	struct MappableMemoryInfo
	{
		void* mBaseAddress;
		vk::DeviceMemory mBackingMemory;
	};
	std::vector<MappableMemoryInfo> mDeviceMemoryBackers;
	std::vector<MappableMemoryInfo> mHostMappableMemoryBackers;
	std::vector<std::list<PoolFragment>>   mDeviceLocalPools;
	std::vector<std::list<PoolFragment>>   mHostMappablePools;
};

#endif
