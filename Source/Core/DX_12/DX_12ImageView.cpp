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
	    D3D12_RENDER_TARGET_VIEW_DESC renderTarget{};
        renderTarget.Format = getDX12ImageFormat(mImageFormat);
        renderTarget.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
        renderTarget.Texture2D = {lod, level};

        // TODO add to cpu descriptor pool
	}

	if (mUsage & ImageUsage::Sampled) // create SRV.
	{
	    D3D12_SHADER_RESOURCE_VIEW_DESC SRV{};
        SRV.Format = getDX12ImageFormat(mImageFormat);
		D3D12_SRV_DIMENSION dimensions = D3D12_SRV_DIMENSION::D3D12_SRV_DIMENSION_TEXTURE1D;
		const ImageExtent extent = parentImage->getExtent(0, 0);
		if (extent.height > 1)
			dimensions = D3D12_SRV_DIMENSION::D3D12_SRV_DIMENSION_TEXTURE2D;
		if (levelCount > 1)
			dimensions = D3D12_SRV_DIMENSION::D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
		if (extent.depth > 1)
			dimensions = D3D12_SRV_DIMENSION::D3D12_SRV_DIMENSION_TEXTURE3D;
        SRV.ViewDimension = dimensions;
		
		if (dimensions == D3D12_SRV_DIMENSION::D3D12_SRV_DIMENSION_TEXTURE1D)
		{
            SRV.Texture1D = { lod, lodCount, 16.0f };
		}
		else if (dimensions == D3D12_SRV_DIMENSION::D3D12_SRV_DIMENSION_TEXTURE2D)
		{
            SRV.Texture2D = {lod, lodCount, level, 16.0f};
		}
		else if (dimensions == D3D12_SRV_DIMENSION::D3D12_SRV_DIMENSION_TEXTURE2DARRAY)
		{
            SRV.Texture2DArray = {lod, lodCount, level, levelCount, level, 16.0f};
		}
		else
		{
            SRV.Texture3D = {lod, lodCount, 16.0f};
		}

        // TODO add to cpu descriptor pool
	}

	if (mUsage & ImageUsage::Storage) // create UAV
	{
	    D3D12_UNORDERED_ACCESS_VIEW_DESC UAV{};
        UAV.Format = getDX12ImageFormat(mImageFormat);
		D3D12_UAV_DIMENSION dimensions = D3D12_UAV_DIMENSION::D3D12_UAV_DIMENSION_TEXTURE1D;
		const ImageExtent extent = parentImage->getExtent(0, 0);
		if (extent.height > 1)
			dimensions = D3D12_UAV_DIMENSION::D3D12_UAV_DIMENSION_TEXTURE2D;
		if (levelCount > 1)
			dimensions = D3D12_UAV_DIMENSION::D3D12_UAV_DIMENSION_TEXTURE2DARRAY;
		if (extent.depth > 1)
			dimensions = D3D12_UAV_DIMENSION::D3D12_UAV_DIMENSION_TEXTURE3D;
        UAV.ViewDimension = dimensions;
		if (dimensions == D3D12_UAV_DIMENSION::D3D12_UAV_DIMENSION_TEXTURE1D)
		{
            UAV.Texture1D = { lod};
		}
		else if (dimensions == D3D12_UAV_DIMENSION::D3D12_UAV_DIMENSION_TEXTURE2D)
		{
            UAV.Texture2D = { lod, level};
		}
		else if (dimensions == D3D12_UAV_DIMENSION::D3D12_UAV_DIMENSION_TEXTURE2DARRAY)
		{
            UAV.Texture2DArray = { lod, level, levelCount, level };
		}
		else
		{
            UAV.Texture3D = { lod, 0,  extent.depth};
		}

        // TODO add to cpu descriptor pool
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