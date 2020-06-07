#include "DX_12ShaderResourceSet.hpp"

#include "DX_12RenderDevice.hpp"


DX_12ShaderResourceSet::DX_12ShaderResourceSet(RenderDevice* dev, const uint32_t maxDescriptors) :
	ShaderResourceSetBase(dev, maxDescriptors)
{

}


DX_12ShaderResourceSet::~DX_12ShaderResourceSet()
{
	getDevice()->destroyShaderResourceSet(*this);
}


void DX_12ShaderResourceSet::finalise()
{

}