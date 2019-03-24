#ifndef GPURESOURCE_HPP
#define GPURESOURCE_HPP

#include <atomic>
#include <vulkan/vulkan.hpp>
#include <cstdint>

enum class ImageType {
    Texture1D = 0,
    Texture2D,
    Texture3D,
    RenderTarget1D,
    RenderTarget2D,
    RenderTarget3D
};

enum class BufferType {
    VertexBuffer = 0,
    IndexBuffer,
    UniformBuffer,
    DataBuffer
};

enum class QueueType {
    Graphics = 0,
    Compute,
    Transfer,
	MaxQueues
};


class RefCount {
public:
    RefCount() : mRefCount{new std::atomic<uint32_t>(1u)} {}

    ~RefCount()
    {
        const uint32_t oldRefCount = std::atomic_fetch_add(mRefCount, 0u);
        if(oldRefCount == 0)
            delete mRefCount;
    }

    RefCount(const RefCount& other)
    {
        mRefCount = other.mRefCount;

        aquire();
    }

    RefCount& operator=(const RefCount& other)
    {
        mRefCount = other.mRefCount;

        aquire();

        return *this;
    }

    RefCount(RefCount&&) = default;
    RefCount& operator=(RefCount&&) = default;

    void                aquire() { std::atomic_fetch_add(mRefCount, 1u); }
    bool                release() { const uint32_t oldRefCount = std::atomic_fetch_sub(mRefCount, 1u); return oldRefCount == 1; }

protected:
    std::atomic<uint32_t>* mRefCount;
};


class GPUResource : public RefCount {
public:
    GPUResource(const uint64_t lastAccessed) :  mNeedsUpdating{false},
                                                mLastAccessed{lastAccessed},
                                                mCurrentQueue{QueueType::Graphics} {}

    GPUResource(const GPUResource& other) : RefCount{other}
    {
        mNeedsUpdating = other.mNeedsUpdating;
        mLastAccessed = other.mLastAccessed;
        mCurrentQueue = other.mCurrentQueue;
    }

    GPUResource& operator=(const GPUResource& other)
    {
        mNeedsUpdating = other.mNeedsUpdating;
        mLastAccessed = other.mLastAccessed;
        mCurrentQueue = other.mCurrentQueue;
        mRefCount = other.mRefCount;

        return *this;
    }

    GPUResource(GPUResource&&) = default;
    GPUResource& operator=(GPUResource&&) = default;

    inline uint64_t		getLastAccessed() const { return mLastAccessed; }
    inline void			updateLastAccessed(const uint64_t updatedAccess) { mLastAccessed = updatedAccess; }

    inline bool			needsUpdating() const { return mNeedsUpdating; }
    inline void			markNeedingUpdate() { mNeedsUpdating = true; }

	inline QueueType	getOwningQueueType() const { return mCurrentQueue; }
	inline void			setOwningQueueType(const QueueType queueType) { mCurrentQueue = queueType; }

private:
    bool mNeedsUpdating;
    uint64_t mLastAccessed;
    QueueType mCurrentQueue;
};

#endif
