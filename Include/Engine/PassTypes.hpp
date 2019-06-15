#ifndef PASSTYPES_HPP
#define PASSTYPES_HPP

#include <cstdint>

// An enum to keep track of which 
enum class PassType : uint64_t
{
	DepthPre = 0,
	GBuffer = 1,
	Animation = 1 << 1,
	DeferredLighting = 1 << 2,
	Shadow = 1 << 3,
	CascadingShadow = 1 << 4,
	SSAO = 1 << 5,
	GBufferMaterial = 1 << 6

	// Add more as and when implemented.
};


enum class AttachmentType
{
	Texture1D,
	Texture2D,
	Texture3D,
	Sampler,
	Image1D,
	Image2D,
	Image3D,
	RenderTarget1D,
	RenderTarget2D,
	RenderTarget3D,
	SwapChain,
	Depth,
	UniformBuffer,
	DataBuffer,
	IndirectBuffer,
	PushConstants
};


enum class Format
{
	RGBA8SRGB,
	RGBA8UNorm,
	BGRA8UNorm,
	D32Float,
	D24S8Float,
	R32Uint,
	R8UNorm,
	R16G16Unorm
};


#endif

