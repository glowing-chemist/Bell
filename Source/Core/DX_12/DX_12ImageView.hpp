#ifndef DX_12IMAGE_VIEW_HPP
#define DX_12IMAGE_VIEW_HPP

#include <d3d12.h>

#include "Core/Image.hpp"
#include "Core/ImageView.hpp"


class DX_12ImageView : public ImageViewBase
{
public:

	DX_12ImageView(Image&,
		const ImageViewType,
		const uint32_t level = 0,
		const uint32_t levelCount = 1,
		const uint32_t lod = 0,
		const uint32_t lodCount = 1);

	~DX_12ImageView();

	D3D12_RENDER_TARGET_VIEW_DESC getRenderTarget() const
	{
		return mRenderTarget;
	}

	D3D12_UNORDERED_ACCESS_VIEW_DESC getUnorderedAccessView() const
	{
		return mUAV;
	}

	D3D12_SHADER_RESOURCE_VIEW_DESC  getShaderResourceView() const 
	{
		return mSRV;
	}

	ID3D12Resource* getResource()
	{
		return mResource;
	}

private:

	D3D12_RENDER_TARGET_VIEW_DESC mRenderTarget;
	D3D12_UNORDERED_ACCESS_VIEW_DESC  mUAV;
	D3D12_SHADER_RESOURCE_VIEW_DESC  mSRV;

	ID3D12Resource* mResource;

	bool mOwned;
};

#endif