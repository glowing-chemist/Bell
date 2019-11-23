#ifndef PASSTYPES_HPP
#define PASSTYPES_HPP

#include <cstdint>


#define PASS_TYPES  DepthPre = 1, \
                    GBuffer = 1 << 1, \
                    Animation = 1 << 2, \
                    DeferredTextureBlinnPhongLighting = 1 << 3, \
                    Shadow = 1 << 4, \
                    CascadingShadow = 1 << 5, \
                    SSAO = 1 << 6, \
                    GBufferMaterial = 1 << 7, \
                    EditorDefault = 1 << 8, \
                    GBufferPreDepth = 1 << 9, \
                    GBUfferMaterialPreDepth = 1 << 10, \
					InplaceCombine = 1 << 11, \
					InplaceCombineSRGB = 1 << 12, \
					DeferredTexturePBRIBL = 1 << 13, \
					Overlay = 1 << 14, \
					Skybox = 1 << 15, \
					ConvolveSkybox = 1 << 16, \
					DFGGeneration = 1 << 17, \
					DeferredTextureAnalyticalPBRIBL = 1 << 18, \
					Composite = 1 << 19 \

// An enum to keep track of which 
enum class PassType : uint64_t
{
    PASS_TYPES

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

        case PassType::GBufferMaterial:
            return "GBuffer Material pass";

        case PassType::SSAO:
            return "SSAO";

        case PassType::GBufferPreDepth:
            return "GBuffer Pre-Depth";

        case PassType::GBUfferMaterialPreDepth:
            return "GBuffer Material Pre-Depth";

		case PassType::InplaceCombine:
			return "InplaceCombine";

		case PassType::InplaceCombineSRGB:
			return "InplaceCombineSRGB";

		case PassType::DeferredTextureBlinnPhongLighting:
			return "DeferredTextureBlinnPhong";

		case PassType::DeferredTexturePBRIBL:
			return "PBR IBL";

		case PassType::DeferredTextureAnalyticalPBRIBL:
			return "Analytical IBL";

		case PassType::Skybox:
			return "skybox";

		case PassType::ConvolveSkybox:
			return "convolveSkybox";

		case PassType::DFGGeneration:
			return "DFGGenration";
    }

    return "UNKNOWN PASS TYPE";
}


enum class AttachmentType
{
	Texture1D = 0,
	Texture2D,
	Texture3D,
	TextureArray,
	Sampler,
	Image1D,
	Image2D,
	Image3D,
	RenderTarget1D,
	RenderTarget2D,
	RenderTarget3D,
	Depth,
	UniformBuffer,
	DataBuffer,
	IndirectBuffer,
	PushConstants,
	ShaderResourceSet
};


enum class Format
{
	RGBA8SRGB,
    RGB8UNorm,
    RGB8SRGB,
	RGB32SFloat,
	RGBA32SFloat,
	RGBA8UNorm,
	BGRA8UNorm,
	D32Float,
	D24S8Float,
	R32Uint,
	R8UNorm,
	RG16UNorm,
	RGB16UNorm,
	RGBA16UNorm,
	RGBA16SNorm,
	RGBA16Float,
	RG32Float,
	RGBA32Float
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

