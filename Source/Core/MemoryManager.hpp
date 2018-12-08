#ifndef MemoryManager_HPP
#define MemoryManager_HPP

#include "DeviceChild.hpp"
#include <vulkan/vulkan.hpp>

#include <vector>
#include <list>

#define MEMORY_LOGGING 0

struct PoolFragment {
    friend class MemoryManager; // to allow this to be an opaque handle that only the memory manager can use
    friend bool operator==(const PoolFragment&, const PoolFragment&);
private:
    uint64_t offset;
    uint64_t size;
    bool DeviceLocal;
    bool free;
    bool canBeMerged = false;
};

struct Allocation {
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
    MemoryManager() = default; // constructor that doens't allocate pools
    explicit MemoryManager(RenderDevice*); // one that does

    Allocation Allocate(uint64_t size, unsigned long allignment, bool hostMappable);
    void       Free(Allocation alloc);

    void       BindImage(vk::Image& image, Allocation alloc);
    void       BindBuffer(vk::Buffer& buffer, Allocation alloc);

	void*	   MapAllocation(Allocation alloc);
	void	   UnMapAllocation(Allocation alloc);

    void       Destroy();

#if MEMORY_LOGGING
    void dumpPools() const;
#endif

private:
    void MergeFreePools();
    void MergePool(std::vector<std::list<PoolFragment>>& pools);

    Allocation AttemptToAllocate(uint64_t size, unsigned int allignment, bool deviceLocal);

    void AllocateDevicePool();
    void AllocateHostMappablePool();

    void FreeDevicePools();
    void FreeHostMappablePools();

    void findPoolIndicies();

    uint32_t mDeviceLocalPoolIndex;
    uint32_t mHostMappablePoolIndex;

    std::vector<vk::DeviceMemory> deviceMemoryBackers;
    std::vector<vk::DeviceMemory> hostMappableMemoryBackers;
    std::vector<std::list<PoolFragment>>   deviceLocalPools;
    std::vector<std::list<PoolFragment>>   hostMappablePools;
};

#endif
