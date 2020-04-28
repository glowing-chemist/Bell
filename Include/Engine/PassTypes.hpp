#ifndef PASSTYPES_HPP
#define PASSTYPES_HPP

#include <cstdint>


#define PASS_TYPES  DepthPre = 1, \
                    GBuffer = 1 << 1, \
                    Animation = 1 << 2, \
                    Shadow = 1 << 3, \
                    CascadingShadow = 1 << 4, \
                    SSAO = 1 << 5, \
                    GBufferMaterial = 1 << 6, \
                    EditorDefault = 1 << 7, \
                    GBufferPreDepth = 1 << 8, \
                    GBUfferMaterialPreDepth = 1 << 9, \
                    InplaceCombine = 1 << 10, \
                    InplaceCombineSRGB = 1 << 11, \
                    Overlay = 1 << 12, \
                    Skybox = 1 << 13, \
                    ConvolveSkybox = 1 << 14, \
                    DFGGeneration = 1 << 15, \
                    Composite = 1 << 16, \
                    ForwardIBL = 1 << 17, \
                    LightFroxelation = 1 << 18, \
                    DeferredPBRIBL = 1 << 19, \
                    DeferredAnalyticalLighting = 1 << 20, \
                    ForwardAnalyticalLighting = 1 << 21, \
                    DeferredCombinedLighting = 1 << 22, \
                    ForwardCombinedLighting = 1 << 23, \
                    SSAOImproved = 1 << 24, \
                    TAA = 1 << 25, \
                    LineariseDepth = 1 << 26, \
                    SSR = 1 << 27, \
                    Voxelize = 1 << 28, \
                    DebugAABB = 1 << 29, \
                    DebugWireFrame = 1 << 30

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

        case PassType::DeferredPBRIBL:
            return "Deferred_PBR_IBL";

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

		case PassType::LineariseDepth:
			return "Linearise_depth";

		case PassType::Overlay:
			return "Overlay";

        case PassType::SSR:
            return "Screen_Space_Reflection";

        case PassType::Composite:
            return "Composite";

        case PassType::DebugAABB:
            return "DebugAABB";

        case PassType::DebugWireFrame:
            return "WireFrame";
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
    RGBA16UInt,
    RGBA8Uint
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

