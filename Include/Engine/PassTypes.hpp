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

inline const char* passToString(const PassType passType)
{
    switch(passType)
    {
        case PassType::DepthPre:
            return "Depth Pre pass";

        case PassType::GBuffer:
            return "GBuffer pass";

        case PassType::SSAO:
            return "SSAO";
    }

    return "UNKNOWN PASS TYPE";
}


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
	RGB32SFloat,
	RGBA32SFloat,
	RGBA8UNorm,
	BGRA8UNorm,
	D32Float,
	D24S8Float,
	R32Uint,
	R8UNorm,
	R16G16Unorm
};


enum class ImageUsage : uint32_t
{
	Sampled = 1,
	ColourAttachment = 1 << 1,
	DepthStencil = 1 << 2,
	Storage = 1 << 3,
	TransferDest = 1 << 4,
	TransferSrc = 1 << 5,
	CubeMap = 1 << 6
};

inline ImageUsage operator|(const ImageUsage& lhs, const ImageUsage& rhs)
{
	return static_cast<ImageUsage>(static_cast<uint32_t>(lhs) | static_cast<uint32_t>(rhs));
}

inline uint32_t operator&(const ImageUsage& lhs, const ImageUsage& rhs)
{
	return static_cast<uint32_t>(lhs) & static_cast<uint32_t>(rhs);
}


#endif

