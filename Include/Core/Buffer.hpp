#ifndef BUFFER_HPP
#define BUFFER_HPP

#include <memory>
#include <string>

#include "Core/GPUResource.hpp"
#include "Core/DeviceChild.hpp"

#include "Engine/PassTypes.hpp"

class RenderDevice;


struct MapInfo
{
	size_t mOffset;
	size_t mSize;
};


class BufferBase : public DeviceChild, public GPUResource
{
public:
    BufferBase(RenderDevice* dev,
           BufferUsage usage,
           const uint32_t size,
           const uint32_t stride,
		   const std::string& = "");

    virtual ~BufferBase() = default;

	virtual void swap(BufferBase&);

    // will only resize to a larger buffer.
    // returns whether or not the buffer was resized (and views need to be recreated)
    virtual bool resize(const uint32_t newSize, const bool preserContents) = 0;

    BufferUsage getUsage() const
        { return mUsage; }

	uint32_t getSize() const
		{ return mSize;}

	virtual uint64_t getDeviceAddress() const = 0;

    virtual void setContents(const void* data,
                     const uint32_t size,
                     const uint32_t offset = 0) = 0;

    // Repeat the data in the range (start + offset, start + offset + size]
    virtual void setContents(const int data,
                     const uint32_t size,
                     const uint32_t offset = 0) = 0;


	virtual void*   map(MapInfo &mapInfo) = 0;
    virtual void    unmap() = 0;

	virtual void updateLastAccessed();

protected:

    inline bool isMappable() const
    {
        return static_cast<bool>(mUsage & BufferUsage::TransferSrc ||
        mUsage & BufferUsage::Uniform);
    }

    BufferUsage mUsage;
    uint32_t mSize;
    uint32_t mStride;
    uint32_t mAllignment;
    std::string mName;
};


class Buffer
{
public:

	Buffer(RenderDevice* dev,
		BufferUsage usage,
		const uint32_t size,
		const uint32_t stride,
		const std::string & = "");
	~Buffer() = default;

	BufferBase* getBase()
	{
		return mBase.get();
	}

	const BufferBase* getBase() const 
	{
		return mBase.get();
	}

	BufferBase* operator->()
	{
		return getBase();
	}

	const BufferBase* operator->() const
	{
		return getBase();
	}

private:

	std::shared_ptr<BufferBase> mBase;

};


#endif
