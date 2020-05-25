#include "DX_12ImageView.hpp"
#include "DX_12Image.hpp"
#include "DX_12RenderDevice.hpp"
#include "Core/ConversionUtils.hpp"


DX_12ImageView::DX_12ImageView(Image& parentImage,
	const ImageViewType type,
	const uint32_t level,
	const uint32_t levelCount ,
	const uint32_t lod,
	const uint32_t lodCount)
	: ImageViewBase(parentImage, type, level, levelCount, lod, lodCount),
	mOwned(true)
{
	mResource = static_cast<DX_12Image*>(parentImage.getBase())->getResourceHandle();
	mResource->AddRef();

	DX_12RenderDevice* device = static_cast<DX_12RenderDevice*>(getDevice());

	if (mUsage & ImageUsage::ColourAttachment || mUsage & ImageUsage::DepthStencil)
	{
		mRenderTarget.Format = getDX12ImageFormat(mImageFormat);
		mRenderTarget.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
		mRenderTarget.Texture2D = {lod, level};
	}

	if (mUsage & ImageUsage::Sampled) // create SRV.
	{
		mSRV.Format = getDX12ImageFormat(mImageFormat);
		D3D12_SRV_DIMENSION dimensions = D3D12_SRV_DIMENSION::D3D12_SRV_DIMENSION_TEXTURE1D;
		const ImageExtent extent = parentImage->getExtent(0, 0);
		if (extent.height > 1)
			dimensions = D3D12_SRV_DIMENSION::D3D12_SRV_DIMENSION_TEXTURE2D;
		if (levelCount > 1)
			dimensions = D3D12_SRV_DIMENSION::D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
		if (extent.depth > 1)
			dimensions = D3D12_SRV_DIMENSION::D3D12_SRV_DIMENSION_TEXTURE3D;
		mSRV.ViewDimension = dimensions;
		
		if (dimensions == D3D12_SRV_DIMENSION::D3D12_SRV_DIMENSION_TEXTURE1D)
		{
			mSRV.Texture1D = { lod, lodCount, 16.0f };
		}
		else if (dimensions == D3D12_SRV_DIMENSION::D3D12_SRV_DIMENSION_TEXTURE2D)
		{
			mSRV.Texture2D = {lod, lodCount, level, 16.0f};
		}
		else if (dimensions == D3D12_SRV_DIMENSION::D3D12_SRV_DIMENSION_TEXTURE2DARRAY)
		{
			mSRV.Texture2DArray = {lod, lodCount, level, levelCount, level, 16.0f};
		}
		else
		{
			mSRV.Texture3D = {lod, lodCount, 16.0f};
		}
	}

	if (mUsage & ImageUsage::Storage) // create UAV
	{
		mUAV.Format = getDX12ImageFormat(mImageFormat);
		D3D12_UAV_DIMENSION dimensions = D3D12_UAV_DIMENSION::D3D12_UAV_DIMENSION_TEXTURE1D;
		const ImageExtent extent = parentImage->getExtent(0, 0);
		if (extent.height > 1)
			dimensions = D3D12_UAV_DIMENSION::D3D12_UAV_DIMENSION_TEXTURE2D;
		if (levelCount > 1)
			dimensions = D3D12_UAV_DIMENSION::D3D12_UAV_DIMENSION_TEXTURE2DARRAY;
		if (extent.depth > 1)
			dimensions = D3D12_UAV_DIMENSION::D3D12_UAV_DIMENSION_TEXTURE3D;
		mUAV.ViewDimension = dimensions;
		if (dimensions == D3D12_UAV_DIMENSION::D3D12_UAV_DIMENSION_TEXTURE1D)
		{
			mUAV.Texture1D = { lod};
		}
		else if (dimensions == D3D12_UAV_DIMENSION::D3D12_UAV_DIMENSION_TEXTURE2D)
		{
			mUAV.Texture2D = { lod, level};
		}
		else if (dimensions == D3D12_UAV_DIMENSION::D3D12_UAV_DIMENSION_TEXTURE2DARRAY)
		{
			mUAV.Texture2DArray = { lod, level, levelCount, level };
		}
		else
		{
			mUAV.Texture3D = { lod, 0,  extent.depth};
		}
	}
}

DX_12ImageView::~DX_12ImageView()
{
	if (mOwned)
	{
		mResource->Release();
		getDevice()->destroyImageView(*this);
	}
}