#ifndef SHADER_RESOURCE_SET_HPP
#define SHADER_RESOURCE_SET_HPP

#include "Core/DeviceChild.hpp"
#include "Core/GPUResource.hpp"
#include "Core/Sampler.hpp"
#include "Core/BufferView.hpp"
#include "Engine/PassTypes.hpp"

#include <memory>
#include <vector>

class ImageView;
using ImageViewArray = std::vector<ImageView>;
using BufferViewArray = std::vector<BufferView>;

class ShaderResourceSetBase : public DeviceChild, public GPUResource
{
public:
	ShaderResourceSetBase(RenderDevice* dev, const uint32_t maxDescriptors);
	virtual ~ShaderResourceSetBase() = default;

	void addSampledImage(const ImageView&);
	void addStorageImage(const ImageView&);
	void addSampledImageArray(const ImageViewArray&);
	void addUniformBuffer(const BufferView&);
    void addDataBufferRO(const BufferView&);
    void addDataBufferRW(const BufferView&);
    void addDataBufferWO(const BufferView&);
    void addDataBufferROArray(const BufferViewArray&);

    void updateLastAccessed();

	virtual void finalise() = 0;

	struct ResourceInfo
	{
		size_t mIndex;
		AttachmentType mType;
        size_t mArraySize;
	};

protected:

	uint32_t mMaxDescriptors;

	std::vector<ImageView> mImageViews;
	std::vector<BufferView> mBufferViews;
	std::vector<ImageViewArray> mImageArrays;
    std::vector<BufferViewArray> mBufferArrays;

	std::vector<ResourceInfo> mResources;
};


class ShaderResourceSet
{
public:

	ShaderResourceSet(RenderDevice* dev, const uint32_t maxDescriptors);
	~ShaderResourceSet() = default;


	ShaderResourceSetBase* getBase()
	{
		return mBase.get();
	}

	const ShaderResourceSetBase* getBase() const
	{
		return mBase.get();
	}

	ShaderResourceSetBase* operator->()
	{
		return getBase();
	}

	const ShaderResourceSetBase* operator->() const
	{
		return getBase();
	}

    void reset(RenderDevice* dev, const uint32_t maxDescriptors);

private:

	std::shared_ptr<ShaderResourceSetBase> mBase;

};


#endif
