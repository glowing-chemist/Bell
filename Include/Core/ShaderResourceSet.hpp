#ifndef SHADER_RESOURCE_SET_HPP
#define SHADER_RESOURCE_SET_HPP

#include "Core/DeviceChild.hpp"
#include "Core/GPUResource.hpp"
#include "Engine/PassTypes.hpp"

#include <memory>
#include <vector>

class ImageView;
class BufferView;
class Sampler;
using ImageViewArray = std::vector<ImageView>;


class ShaderResourceSetBase : public DeviceChild, public GPUResource
{
public:
	ShaderResourceSetBase(RenderDevice* dev);
	virtual ~ShaderResourceSetBase() = default;

	void addSampledImage(const ImageView&);
	void addStorageImage(const ImageView&);
	void addSampledImageArray(const ImageViewArray&);
	void addSampler(const Sampler&);
	void addUniformBuffer(const BufferView&);
    void addDataBufferRO(const BufferView&);
    void addDataBufferRW(const BufferView&);
    void addDataBufferWO(const BufferView&);
	virtual void finalise() = 0;

	struct ResourceInfo
	{
		size_t mIndex;
		AttachmentType mType;
	};

protected:

	std::vector<ImageView> mImageViews;
	std::vector<BufferView> mBufferViews;
	std::vector<Sampler>   mSamplers;
	std::vector<ImageViewArray> mImageArrays;

	std::vector<ResourceInfo> mResources;
};


class ShaderResourceSet
{
public:

	ShaderResourceSet(RenderDevice* dev);
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

private:

	std::shared_ptr<ShaderResourceSetBase> mBase;

};


#endif
