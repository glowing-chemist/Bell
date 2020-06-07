#ifndef DX_12SRS_HPP
#define DX_12SRS_HPP

#include "Core/ShaderResourceSet.hpp"

#include <d3d12.h>
#include <vector>


class DX_12ShaderResourceSet : public ShaderResourceSetBase
{
public:
	DX_12ShaderResourceSet(RenderDevice*);
	~DX_12ShaderResourceSet();

	virtual void finalise() override final;

private:

	ID3D12DescriptorHeap* mHeap;
	std::vector<uint32_t> mDescriptorOffsets;
};

#endif