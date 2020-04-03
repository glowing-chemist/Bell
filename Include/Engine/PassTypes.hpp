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
					Composite = 1 << 19, \
					ForwardIBL = 1 << 20, \
                    LightFroxelation = 1 << 21, \
                    DeferredPBRIBL = 1 << 22, \
					DeferredAnalyticalLighting = 1 << 23, \
					ForwardAnalyticalLighting = 1 << 24, \
					DeferredCombinedLighting = 1 << 25, \
					ForwardCombinedLighting = 1 << 26, \
					SSAOImproved = 1 << 27, \
					TAA = 1 << 28, \
					LineariseDepth = 1 << 29

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
            return "Depth_Pre_pass";

        case PassType::GBuffer:
            return "GBuffer_pass";

        case PassType::GBufferMaterial:
            return "GBuffer_Material_pass";

        case PassType::SSAO:
            return "SSAO";

		case PassType::SSAOImproved:
			return "SSAO_Improved";

        case PassType::GBufferPreDepth:
            return "GBuffer_Pre_Depth";

        case PassType::GBUfferMaterialPreDepth:
            return "GBuffer_Material_Pre_Depth";

		case PassType::InplaceCombine:
			return "Inplace_combine";

		case PassType::InplaceCombineSRGB:
			return "Inplace_combineSRGB";

		case PassType::DeferredTextureBlinnPhongLighting:
			return "Deferred_texture_blinnPhong";

		case PassType::DeferredTexturePBRIBL:
            return "Deferred_Texturing_IBL";

        case PassType::DeferredPBRIBL:
            return "Deferred_PBR_IBL";

		case PassType::DeferredTextureAnalyticalPBRIBL:
            return "Analytical_deferred_texturing_IBL";

		case PassType::ForwardIBL:
			return "ForwardIBL";

		case PassType::ForwardCombinedLighting:
			return "Forward_combined_lighting";

		case PassType::Skybox:
			return "Skybox";

		case PassType::ConvolveSkybox:
			return "Convolve_skybox";

		case PassType::DFGGeneration:
			return "DFGGenration";

		case PassType::LightFroxelation:
			return "Light_froxelation";

		case PassType::DeferredAnalyticalLighting:
			return "Deferred_analytical_lighting";

        case PassType::Shadow:
            return "Shadow_Map";

		case PassType::TAA:
			return "TAA";
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
    DataBufferRO,
    DataBufferWO,
    DataBufferRW,
	IndirectBuffer,
    VertexBuffer,
    IndexBuffer,
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
	R32Float,
	R8UNorm,
    RG8UNorm,
	RG16UNorm,
	RGB16UNorm,
	RGBA16UNorm,
	RGBA16SNorm,
	RGBA16Float,
	RG32Float,
	RGBA32Float,
	R16UInt,
	RGBA16UInt
};


enum class ImageUsage : uint32_t
{
	Sampled = 1,
	ColourAttachment = 1 << 1,
	DepthStencil = 1 << 2,
	Storage = 1 << 3,
	TransferDest = 1 << 4,
	TransferSrc = 1 << 5,
	CubeMap = 1 << 6,
	Transient = 1 << 7
};

inline ImageUsage operator|(const ImageUsage& lhs, const ImageUsage& rhs)
{
	return static_cast<ImageUsage>(static_cast<uint32_t>(lhs) | static_cast<uint32_t>(rhs));
}

inline uint32_t operator&(const ImageUsage& lhs, const ImageUsage& rhs)
{
	return static_cast<uint32_t>(lhs) & static_cast<uint32_t>(rhs);
}


enum class BufferUsage : uint32_t
{
	Vertex = 1,
	Index = 1 << 1,
	Uniform = 1 << 2,
	DataBuffer = 1 << 3,
	IndirectArgs = 1 << 4,
	TransferDest = 1 << 5,
	TransferSrc = 1 << 6
};

inline BufferUsage operator|(const BufferUsage& lhs, const BufferUsage& rhs)
{
	return static_cast<BufferUsage>(static_cast<uint32_t>(lhs) | static_cast<uint32_t>(rhs));
}

inline uint32_t operator&(const BufferUsage& lhs, const BufferUsage& rhs)
{
	return static_cast<uint32_t>(lhs)& static_cast<uint32_t>(rhs);
}


#endif

