#ifndef PASSTYPES_HPP
#define PASSTYPES_HPP

#include <cstdint>

// An enum to keep track of which 
enum class PassType : uint64_t
{
	DepthPre = 0,
	GBufferFill = 1,
	Animation = 1 << 1,
	DeferredLighting = 1 << 2,
	Shadow = 1 << 3,
	CascadingShadow = 1 << 4

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
	D24S8Float
};


#endif

